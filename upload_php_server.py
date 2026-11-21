#!/usr/bin/env python3
"""
Upload PHP server to www.familie-riegel.de/PetDuet via SFTP.
Uses .env for credentials (same pattern as D:\Development\ESP32\TID).
"""

import os
import sys
from pathlib import Path

try:
    from dotenv import load_dotenv
except ImportError:
    print("Installing python-dotenv...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "python-dotenv"])
    from dotenv import load_dotenv

try:
    import paramiko
except ImportError:
    print("Installing paramiko...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "paramiko"])
    import paramiko

# Load environment variables
env_path = Path(__file__).parent / ".env"
if not env_path.exists():
    print(f"Error: .env file not found at {env_path}")
    print("Copy .env.example to .env and fill in your credentials.")
    sys.exit(1)

load_dotenv(env_path)

FTP_HOST = os.getenv("FTP_HOST")
FTP_PORT = int(os.getenv("FTP_PORT", "22"))
FTP_USER = os.getenv("FTP_USER")
FTP_PASSWORD = os.getenv("FTP_PASSWORD")
FTP_REMOTE_DIR = os.getenv("FTP_REMOTE_DIR", "/PetDuet/")

LOCAL_DIR = Path(__file__).parent / "php_server"

EXCLUDE = {
    ".git",
    ".gitignore",
    "__pycache__",
    "*.pyc",
    ".env",
    ".env.example",
    "*.zip",
    "cyd_devices.json",  # runtime file, don't deploy
}

def should_exclude(name: str) -> bool:
    if name in EXCLUDE:
        return True
    for pattern in EXCLUDE:
        if pattern.startswith("*") and name.endswith(pattern[1:]):
            return True
    return False

def ensure_remote_dir(sftp: paramiko.SFTPClient, remote_path: str):
    """Create remote directory recursively."""
    parts = remote_path.strip("/").split("/")
    current = ""
    for part in parts:
        current += "/" + part
        try:
            sftp.stat(current)
        except FileNotFoundError:
            sftp.mkdir(current)
            print(f"Created remote dir: {current}")

def upload_file(sftp: paramiko.SFTPClient, local_path: Path, remote_path: str):
    """Upload a single file."""
    try:
        sftp.put(str(local_path), remote_path)
        print(f"Uploaded: {local_path.relative_to(LOCAL_DIR)} -> {remote_path}")
    except Exception as e:
        print(f"Failed to upload {local_path}: {e}")

def upload_directory(sftp: paramiko.SFTPClient, local_dir: Path, remote_base: str):
    """Recursively upload directory contents."""
    for item in local_dir.iterdir():
        if should_exclude(item.name):
            continue

        remote_path = f"{remote_base.rstrip('/')}/{item.name}"

        if item.is_file():
            upload_file(sftp, item, remote_path)
        elif item.is_dir():
            try:
                sftp.stat(remote_path)
            except FileNotFoundError:
                sftp.mkdir(remote_path)
            upload_directory(sftp, item, remote_path)

def main():
    print(f"Connecting to {FTP_HOST}:{FTP_PORT} as {FTP_USER}...")

    transport = paramiko.Transport((FTP_HOST, FTP_PORT))
    transport.connect(username=FTP_USER, password=FTP_PASSWORD)
    sftp = paramiko.SFTPClient.from_transport(transport)

    try:
        print(f"Ensuring remote directory: {FTP_REMOTE_DIR}")
        ensure_remote_dir(sftp, FTP_REMOTE_DIR)

        print(f"Uploading {LOCAL_DIR} to {FTP_REMOTE_DIR}...")
        upload_directory(sftp, LOCAL_DIR, FTP_REMOTE_DIR)

        print("\n✅ Upload complete!")
        print(f"Remote URL: https://www.familie-riegel.de/PetDuet/")

    finally:
        sftp.close()
        transport.close()

if __name__ == "__main__":
    main()