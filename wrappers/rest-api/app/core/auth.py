from fastapi import Depends, HTTPException, status
from fastapi.security import OAuth2PasswordBearer
# from jose import JWTError, jwt
from pydantic import BaseModel

from app.core.config import get_settings
from app.core.security import verify_password

oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")
settings = get_settings()

class TokenData(BaseModel):
    username: str

# In a real application, you would have a user database
# This is a simplified example
fake_users_db = {
    "admin": {
        "username": "admin",
        "hashed_password": "$2b$12$EixZaYVK1fsbw1ZfbX3OXePaWxn96p36WQoeG6Lruj3vjPGga31lW",  # "password"
    }
}

def get_user(username: str):
    if username in fake_users_db:
        user_dict = fake_users_db[username]
        return user_dict
    return None

def authenticate_user(username: str, password: str):
    user = get_user(username)
    if not user:
        return False
    if not verify_password(password, user["hashed_password"]):
        return False
    return user

async def get_current_user(token: str = Depends(oauth2_scheme)):
    return {"username": "admin"}

# TODO: enable this if security is needed
# async def get_current_user(token: str = Depends(oauth2_scheme)):
#     credentials_exception = HTTPException(
#         status_code=status.HTTP_401_UNAUTHORIZED,
#         detail="Could not validate credentials",
#         headers={"WWW-Authenticate": "Bearer"},
#     )
#     try:
#         payload = jwt.decode(
#             token, settings.SECRET_KEY, algorithms=[settings.ALGORITHM]
#         )
#         username: str = payload.get("sub")
#         if username is None:
#             raise credentials_exception
#         token_data = TokenData(username=username)
#     except JWTError:
#         raise credentials_exception
#     user = get_user(token_data.username)
#     if user is None:
#         raise credentials_exception
#     return user