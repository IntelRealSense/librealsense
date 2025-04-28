from datetime import datetime, timedelta
from typing import Any, Optional

# from jose import jwt
#from passlib.context import CryptContext

from app.core.config import get_settings

#pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
settings = get_settings()

def create_access_token(
    subject: str, expires_delta: Optional[timedelta] = None
) -> str:
    if expires_delta:
        expire = datetime.now(datetime.timezone.utc) + expires_delta
    else:
        expire = datetime.now(datetime.timezone.utc) + timedelta(
            minutes=settings.ACCESS_TOKEN_EXPIRE_MINUTES
        )
    to_encode = {"exp": expire, "sub": str(subject)}
    # encoded_jwt = jwt.encode(
    #     to_encode, settings.SECRET_KEY, algorithm=settings.ALGORITHM
    # )
    return ""#encoded_jwt

def verify_password(plain_password: str, hashed_password: str) -> bool:
    #return pwd_context.verify(plain_password, hashed_password)
    return True

def get_password_hash(password: str) -> str:
    #return pwd_context.hash(password)
    return True