# ./GameV2/logger.py
# Modular logging system with configurable toggles via a text file

import logging
import os

# Config file path relative to this script’s directory
CONFIG_FILE = os.path.join(os.path.dirname(__file__), "logging_config.txt")

# Default config content if file doesn’t exist
DEFAULT_CONFIG = """# Logging configuration
# Set to 1 to enable, 0 to disable
ENABLE_LOGGING=1
LOG_LEVEL=INFO  # Options: DEBUG, INFO, WARNING, ERROR, CRITICAL
LOG_TO_FILE=0   # Set to 1 to log to a file (logs/output.log)
MAP_GEN_LOGS=0  # Enable logs specific to map generation
CREATURE_LOGS=0 # Enable logs specific to creature spawning/behavior
SHOW_SEAM=0     # Show map seam lines (1 = show, 0 = hide)
"""

# Set up the logger
logger = logging.getLogger("GameLogger")
logger.setLevel(logging.DEBUG)  # Base level, filtered by config later

# Console handler
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)  # Will be adjusted by config
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
console_handler.setFormatter(formatter)
logger.addHandler(console_handler)

# File handler (optional, toggled via config)
file_handler = None

# Global config dict
logger.config = {"ENABLE_LOGGING": "0", "LOG_LEVEL": "INFO", "LOG_TO_FILE": "0", 
                 "MAP_GEN_LOGS": "0", "CREATURE_LOGS": "0", "SHOW_SEAM": "0"}

def load_config():
    """Load or create the logging configuration from/to a text file."""
    global file_handler

    abs_path = os.path.abspath(CONFIG_FILE)
    if not os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, 'w') as f:
            f.write(DEFAULT_CONFIG)
        logger.warning(f"Created default config file at {abs_path}")

    config = {"ENABLE_LOGGING": "0", "LOG_LEVEL": "INFO", "LOG_TO_FILE": "0", 
              "MAP_GEN_LOGS": "0", "CREATURE_LOGS": "0", "SHOW_SEAM": "0"}
    try:
        with open(CONFIG_FILE, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    key, value = line.split('=', 1)
                    value = value.split('#', 1)[0].strip()  # Strip comments
                    config[key.strip()] = value
    except Exception as e:
        logger.error(f"Failed to parse config file: {e}")
        return

    enable_logging = config["ENABLE_LOGGING"] == "1"
    log_level = getattr(logging, config["LOG_LEVEL"].upper(), logging.INFO)
    log_to_file = config["LOG_TO_FILE"] == "1"

    logger.handlers = []
    if enable_logging:
        logger.addHandler(console_handler)
        console_handler.setLevel(log_level)
        if log_to_file:
            if file_handler is None:
                file_handler = logging.FileHandler("logs/output.log")
                file_handler.setFormatter(formatter)
                file_handler.setLevel(log_level)
            logger.addHandler(file_handler)
    else:
        logger.handlers = []

    logger.config = config

def log_map_gen(message, level="INFO"):
    """Log messages related to map generation, if enabled."""
    if logger.config.get("MAP_GEN_LOGS") == "1":
        logger.log(getattr(logging, level.upper()), f"[MAP_GEN] {message}")

def log_creature(message, level="INFO"):
    """Log messages related to creatures, if enabled."""
    if logger.config.get("CREATURE_LOGS") == "1":
        logger.log(getattr(logging, level.upper()), f"[CREATURE] {message}")

def debug(message):
    logger.debug(message)

def info(message):
    logger.info(message)

def warning(message):
    logger.warning(message)

def error(message):
    logger.error(message)

def show_seam():
    """Return True if seam should be shown, False otherwise."""
    return logger.config.get("SHOW_SEAM", "0") == "1"

# Load config on import
load_config()

if __name__ == "__main__":
    debug("This is a debug message")
    info("This is an info message")
    warning("This is a warning message")
    error("This is an error message")
    log_map_gen("Testing map generation log")
    log_creature("Testing creature log")
    info(f"SHOW_SEAM: {show_seam()}")