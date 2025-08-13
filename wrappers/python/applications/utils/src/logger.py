import datetime
import logging
import os
import sys

# import termcolor

# if os.name == "nt":  # Windows
#     import colorama
#     colorama.init()


# COLORS = {
#     "WARNING": "yellow",
#     "INFO": "white",
#     "DEBUG": "blue",
#     "CRITICAL": "red",
#     "ERROR": "red",
# }


# class ColoredFormatter(logging.Formatter):
#     def __init__(self, fmt, use_color=True):
#         logging.Formatter.__init__(self, fmt)
#         self.use_color = use_color

#     def format(self, record):
#         levelname = record.levelname
#         if self.use_color and levelname in COLORS:

#             def colored(text):
#                 return termcolor.colored(
#                     text,
#                     color=COLORS[levelname],
#                     attrs={"bold": True},
#                 )

#             record.levelname2 = colored("{:<7}".format(record.levelname))
#             record.message2 = colored(record.msg)

#             asctime2 = datetime.datetime.fromtimestamp(record.created)
#             record.asctime2 = termcolor.colored(asctime2, color="green")

#             record.module2 = termcolor.colored(record.module, color="cyan")
#             record.funcName2 = termcolor.colored(record.funcName, color="cyan")
#             record.lineno2 = termcolor.colored(record.lineno, color="cyan")
#         return logging.Formatter.format(self, record)


log = logging.getLogger("robot")
log.setLevel(logging.DEBUG)

if len(log.handlers)<1:
    print('Logger activated')
    #formatter   = logging.Formatter('[%(asctime)s.%(msecs)03d] {%(filename)6s:%(lineno)3d} %(levelname)s - %(message)s', datefmt="%M:%S", style="{")
    formatter   = logging.Formatter('[%(asctime)s] - [%(filename)28s:%(lineno)4d] - %(levelname)5s - %(message)s')
    log.setLevel("DEBUG")

    console_handler = logging.StreamHandler() # sys.stderr
    console_handler.setLevel("DEBUG")
    console_handler.setFormatter(formatter)
    log.addHandler(console_handler)

    # console_handler = logging.StreamHandler(sys.stderr)
    # handler_format = ColoredFormatter("%(asctime)s [%(levelname2)s] %(module2)s:%(funcName2)s:%(lineno2)s - %(message2)s")
    # console_handler.setFormatter(handler_format)
    # log.addHandler(console_handler)    

    # file_handler = logging.FileHandler("main_app.log", mode="a", encoding="utf-8")
    # file_handler.setLevel("WARNING")
    # file_handler.setFormatter(formatter)
    # logger.addHandler(file_handler)


