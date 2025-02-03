# ./ServerMessage/tabs/__init__.py

from .chat_tab import render_chat_tab
from .history_tab import render_history_tab
from .git_tab import render_git_tab

__all__ = ['render_chat_tab', 'render_history_tab', 'render_git_tab']