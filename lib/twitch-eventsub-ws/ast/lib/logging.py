import logging
import sys

from colorama import Fore, Style


def init_logging(level=logging.DEBUG) -> None:
    root = logging.getLogger()
    root.setLevel(level)

    handler = logging.StreamHandler(sys.stderr)
    handler.setLevel(level)

    colors = {
        "WARNING": Fore.YELLOW,
        "INFO": Fore.WHITE,
        "DEBUG": Fore.BLUE,
        "CRITICAL": Fore.YELLOW,
        "ERROR": Fore.RED,
    }

    class ColoredFormatter(logging.Formatter):
        def format(self, record: logging.LogRecord) -> str:
            levelname = record.levelname
            if levelname in colors:
                levelname_color = Style.BRIGHT + colors[levelname] + levelname + Style.RESET_ALL
                record.levelname = levelname_color
            return logging.Formatter.format(self, record)

    colored_formatter = ColoredFormatter("%(asctime)s  %(levelname)-16s %(name)s: %(message)s")
    handler.setFormatter(colored_formatter)
    root.addHandler(handler)
