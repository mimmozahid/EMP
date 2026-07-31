from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

db = SQLAlchemy()

class AdminManager(db.Model):
    __tablename__ = 'admin_managers'
    
    username = db.Column(db.String(50), primary_key=True)
    email = db.Column(db.String(100), nullable=False)
    password_hash = db.Column(db.String(255), nullable=False)
    role = db.Column(db.Integer, nullable=False) # 1 = Admin, 2 = Manager

    def to_dict(self):
        return {
            'username': self.username,
            'email': self.email,
            'role': self.role,
            'role_name': 'Admin' if self.role == 1 else 'Manager'
        }

class Employee(db.Model):
    __tablename__ = 'employees'
    
    id = db.Column(db.Integer, primary_key=True) # E.g., 1001, 1002
    name = db.Column(db.String(100), nullable=False)
    email = db.Column(db.String(100), nullable=False)
    date_of_birth = db.Column(db.String(20), nullable=False) # DD/MM/YYYY
    position = db.Column(db.String(100), nullable=False)
    base_salary = db.Column(db.Float, nullable=False, default=0.0)
    password_hash = db.Column(db.String(255), nullable=False)
    bonus = db.Column(db.Float, nullable=False, default=0.0)
    deduction = db.Column(db.Float, nullable=False, default=0.0)

    def calculate_salary(self, present_days):
        daily_salary = self.base_salary / 30.0
        final_salary = (daily_salary * present_days) + self.bonus - self.deduction
        return max(0.0, round(final_salary, 2))

    def to_dict(self, present_days=0):
        return {
            'id': self.id,
            'name': self.name,
            'email': self.email,
            'date_of_birth': self.date_of_birth,
            'position': self.position,
            'base_salary': round(self.base_salary, 2),
            'bonus': round(self.bonus, 2),
            'deduction': round(self.deduction, 2),
            'present_days': present_days,
            'final_salary': self.calculate_salary(present_days)
        }

class Attendance(db.Model):
    __tablename__ = 'attendance'
    
    id = db.Column(db.Integer, primary_key=True, autoincrement=True)
    emp_id = db.Column(db.Integer, db.ForeignKey('employees.id'), nullable=False)
    date = db.Column(db.String(20), nullable=False) # DD/MM/YYYY
    status = db.Column(db.Integer, nullable=False) # 1 = Present, 0 = Absent

    def to_dict(self):
        return {
            'id': self.id,
            'emp_id': self.emp_id,
            'date': self.date,
            'status': self.status,
            'status_name': 'Present' if self.status == 1 else 'Absent'
        }

class Leave(db.Model):
    __tablename__ = 'leaves'
    
    id = db.Column(db.Integer, primary_key=True, autoincrement=True)
    emp_id = db.Column(db.Integer, db.ForeignKey('employees.id'), nullable=False)
    apply_date = db.Column(db.String(20), nullable=False) # DD/MM/YYYY
    from_date = db.Column(db.String(20), nullable=False)
    to_date = db.Column(db.String(20), nullable=False)
    reason = db.Column(db.Text, nullable=False)
    status = db.Column(db.Integer, nullable=False, default=0) # 0 = Pending, 1 = Approved, 2 = Rejected

    def to_dict(self):
        status_map = {0: 'Pending', 1: 'Approved', 2: 'Rejected'}
        return {
            'id': self.id,
            'emp_id': self.emp_id,
            'apply_date': self.apply_date,
            'from_date': self.from_date,
            'to_date': self.to_date,
            'reason': self.reason,
            'status': self.status,
            'status_name': status_map.get(self.status, 'Unknown')
        }

class LoginAttempt(db.Model):
    __tablename__ = 'login_attempts'
    
    identifier = db.Column(db.String(100), primary_key=True) # username or emp_id as string
    attempts = db.Column(db.Integer, nullable=False, default=0)
    last_attempt = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

class ActivityLog(db.Model):
    __tablename__ = 'activity_logs'
    
    id = db.Column(db.Integer, primary_key=True, autoincrement=True)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    user_role = db.Column(db.String(20), nullable=False) # Admin, Manager, Employee
    username = db.Column(db.String(100), nullable=False)
    action = db.Column(db.String(100), nullable=False)
    details = db.Column(db.Text, nullable=True)

    def to_dict(self):
        return {
            'id': self.id,
            'timestamp': self.timestamp.strftime('%Y-%m-%d %H:%M:%S'),
            'time_ago': self.get_time_ago(),
            'user_role': self.user_role,
            'username': self.username,
            'action': self.action,
            'details': self.details
        }

    def get_time_ago(self):
        diff = datetime.utcnow() - self.timestamp
        seconds = int(diff.total_seconds())
        if seconds < 60:
            return f"{seconds}s ago"
        elif seconds < 3600:
            return f"{seconds // 60}m ago"
        elif seconds < 86400:
            return f"{seconds // 3600}h ago"
        else:
            return f"{seconds // 86400}d ago"
