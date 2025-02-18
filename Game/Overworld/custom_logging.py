# ./Game/Overworld/custom_logging.py

import logging
import sys

# Logging toggle
LOGGING_ENABLED = False

# Logging setup
if LOGGING_ENABLED:
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.StreamHandler(sys.stdout)
        ]
    )
else:
    logging.basicConfig(level=logging.CRITICAL)

def log(level, message):
    if LOGGING_ENABLED:
        if level == 'DEBUG':
            logging.debug(message)
        elif level == 'INFO':
            logging.info(message)
        elif level == 'WARNING':
            logging.warning(message)
        elif level == 'ERROR':
            logging.error(message)
        elif level == 'CRITICAL':
            logging.critical(message)

# Add some utility functions for common logging patterns
def log_function_call(func_name, *args, **kwargs):
    log('DEBUG', f"Calling {func_name} with args: {args}, kwargs: {kwargs}")

def log_object_creation(class_name, *args, **kwargs):
    log('DEBUG', f"Creating {class_name} with args: {args}, kwargs: {kwargs}")

def log_method_call(class_name, method_name, *args, **kwargs):
    log('DEBUG', f"Calling {class_name}.{method_name} with args: {args}, kwargs: {kwargs}")
