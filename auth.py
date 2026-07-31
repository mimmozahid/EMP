import re
from argon2 import PasswordHasher
from argon2.exceptions import VerifyMismatchError, InvalidHashError
from werkzeug.security import generate_password_hash, check_password_hash
from models import db, LoginAttempt, ActivityLog
from datetime import datetime

ph = PasswordHasher()

def hash_password(password):
    """Hash password using Argon2id"""
    try:
        return ph.hash(password)
    except Exception:
        return generate_password_hash(password)

def verify_password(stored_hash, password):
    """Verify password against Argon2id hash or Werkzeug hash"""
    if not stored_hash or not password:
        return False
    
    if stored_hash.startswith('$argon2'):
        try:
            return ph.verify(stored_hash, password)
        except (VerifyMismatchError, InvalidHashError):
            return False
        except Exception:
            return False
    else:
        try:
            return check_password_hash(stored_hash, password)
        except Exception:
            return stored_hash == password

def create_strong_password(password):
    """
    Validates strong password rules (matching C implementation):
    - Minimum 8 characters
    - At least 1 uppercase letter
    - At least 1 lowercase letter
    - At least 1 digit
    - At least 1 special punctuation character
    """
    if len(password) < 8:
        return False, "Password must be at least 8 characters long."
    
    has_upper = any(c.isupper() for c in password)
    has_lower = any(c.islower() for c in password)
    has_digit = any(c.isdigit() for c in password)
    has_special = any(not c.isalnum() for c in password)
    
    if not (has_upper and has_lower and has_digit and has_special):
        return False, "Password must include uppercase, lowercase, numbers, and special characters."
    
    return True, "Strong password"

def is_valid_email(email):
    """
    Email validation rule from C implementation:
    Must end with @gmail.com or @diu.edu.bd and have characters before @
    """
    if not email or '@' not in email:
        return False
    
    parts = email.split('@')
    if len(parts) != 2 or not parts[0]:
        return False
    
    return email.endswith('@gmail.com') or email.endswith('@diu.edu.bd')

def get_failed_attempts(identifier):
    try:
        record = LoginAttempt.query.get(str(identifier))
        return record.attempts if record else 0
    except Exception as e:
        db.session.rollback()
        print(f"Error getting failed attempts: {e}")
        return 0

def increase_failed_attempts(identifier):
    try:
        record = LoginAttempt.query.get(str(identifier))
        if not record:
            record = LoginAttempt(identifier=str(identifier), attempts=1, last_attempt=datetime.utcnow())
            db.session.add(record)
        else:
            record.attempts += 1
            record.last_attempt = datetime.utcnow()
        db.session.commit()
        return record.attempts
    except Exception as e:
        db.session.rollback()
        print(f"Error increasing failed attempts: {e}")
        return 1

def reset_failed_attempts(identifier):
    try:
        record = LoginAttempt.query.get(str(identifier))
        if record:
            record.attempts = 0
            db.session.commit()
    except Exception as e:
        db.session.rollback()
        print(f"Error resetting failed attempts: {e}")

def log_activity(user_role, username, action, details=None):
    """Log user activity for live activity updates"""
    try:
        log = ActivityLog(
            user_role=user_role,
            username=str(username),
            action=action,
            details=details,
            timestamp=datetime.utcnow()
        )
        db.session.add(log)
        db.session.commit()
    except Exception as e:
        db.session.rollback()
        print(f"Error logging activity: {e}")

