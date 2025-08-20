from functools import lru_cache
from config import Settings, settings as global_settings

@lru_cache()
def get_settings() -> Settings:
    return global_settings
