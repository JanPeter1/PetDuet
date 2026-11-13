<?php
/**
 * Per-request PHP Code Coverage Auto-Prepend Hook
 * Supports PCOV (pcov\start/stop/collect) and Xdebug (xdebug_start_code_coverage).
 */
$__petduetCovOut = getenv('PETDUET_COV_LOG');
$__petduetMode = 'none';

if ($__petduetCovOut) {
    if (extension_loaded('pcov') && function_exists('pcov\\start')) {
        $__petduetMode = 'pcov';
        \pcov\start();
    } elseif (extension_loaded('xdebug') && function_exists('xdebug_start_code_coverage')) {
        $__petduetMode = 'xdebug';
        xdebug_start_code_coverage(XDEBUG_CC_UNUSED | XDEBUG_CC_DEAD_CODE);
    }

    register_shutdown_function(static function () use ($__petduetCovOut, $__petduetMode) {
        if ($__petduetMode === 'none') {
            return;
        }
        $raw = [];
        if ($__petduetMode === 'pcov') {
            \pcov\stop();
            $waiting = \pcov\waiting();
            if (!empty($waiting)) {
                $raw = \pcov\collect(\pcov\inclusive, $waiting);
            }
        } elseif ($__petduetMode === 'xdebug') {
            $raw = xdebug_get_code_coverage();
            xdebug_stop_code_coverage();
        }

        $fp = @fopen($__petduetCovOut, 'c+');
        if (!$fp) {
            return;
        }
        flock($fp, LOCK_EX);
        $size = filesize($__petduetCovOut);
        $existing = $size > 0 ? json_decode(fread($fp, $size), true) : [];
        if (!is_array($existing)) {
            $existing = ['mode' => $__petduetMode, 'files' => []];
        }
        $existing['mode'] = $__petduetMode;

        foreach ($raw as $file => $lines) {
            $norm = str_replace('\\', '/', $file);
            if (!isset($existing['files'][$norm])) {
                $existing['files'][$norm] = [];
            }
            foreach ($lines as $lineNo => $flag) {
                $prev = $existing['files'][$norm][(string)$lineNo] ?? 0;
                // Keep strongest coverage flag: 1 = executed beats -1/-2 (unexecuted/dead)
                if ($flag === 1 || $prev === 0) {
                    $existing['files'][$norm][(string)$lineNo] = $flag;
                }
            }
        }

        ftruncate($fp, 0);
        rewind($fp);
        fwrite($fp, json_encode($existing));
        fflush($fp);
        flock($fp, LOCK_UN);
        fclose($fp);
    });
}
