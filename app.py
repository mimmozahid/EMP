import os
from datetime import datetime
from flask import Flask, render_template, request, jsonify, session, redirect, url_for
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)
app.secret_key = os.environ.get('SECRET_KEY', 'employee-system-secret-key-2026')

# ---------------------------------------------------------------------------
# Database Configuration (TiDB MySQL with SQLite Fallback)
# ---------------------------------------------------------------------------
tidb_host = os.environ.get('TIDB_HOST')
tidb_user = os.environ.get('TIDB_USER')
tidb_password = os.environ.get('TIDB_PASSWORD')
tidb_database = os.environ.get('TIDB_DATABASE')
tidb_port = os.environ.get('TIDB_PORT', '4000')

db_url = os.environ.get('DATABASE_URL')

if not db_url and tidb_host and tidb_user and tidb_password and tidb_database:
    ssl_ca = os.environ.get('TIDB_SSL_CA')
    ssl_query = f"?ssl_ca={ssl_ca}" if ssl_ca else ""
    db_url = f"mysql+pymysql://{tidb_user}:{tidb_password}@{tidb_host}:{tidb_port}/{tidb_database}{ssl_query}"

if not db_url:
    # Local fallback sqlite database
    db_file = os.path.join(app.root_path, 'employee_system.db')
    db_url = f"sqlite:///{db_file}"
    print(f"Using local SQLite database: {db_file}")
else:
    print("Using TiDB MySQL database connection.")

app.config['SQLALCHEMY_DATABASE_URI'] = db_url
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

from models import db, AdminManager, Employee, Attendance, Leave, LoginAttempt, ActivityLog
from auth import (
    hash_password, verify_password, create_strong_password, is_valid_email,
    get_failed_attempts, increase_failed_attempts, reset_failed_attempts, log_activity
)
from seed import seed_database

db.init_app(app)

# Initialize database and seed data
with app.app_context():
    seed_database(app)

# Helper function to get present days for an employee
def count_present_days(emp_id):
    return Attendance.query.filter_by(emp_id=emp_id, status=1).count()

# ---------------------------------------------------------------------------
# Auth Helper & Decorators
# ---------------------------------------------------------------------------
def current_user():
    if 'user_id' not in session:
        return None
    return {
        'id': session.get('user_id'),
        'username': session.get('username'),
        'role': session.get('role'),
        'emp_id': session.get('emp_id')
    }

# ---------------------------------------------------------------------------
# Page Routes
# ---------------------------------------------------------------------------
@app.route('/')
@app.route('/login')
def login_page():
    user = current_user()
    if user:
        return redirect(url_for('dashboard'))
    return render_template('login.html')

@app.route('/dashboard')
def dashboard():
    user = current_user()
    if not user:
        return redirect(url_for('login_page'))
    
    if user['role'] == 1:
        return redirect(url_for('admin_dashboard'))
    elif user['role'] == 2:
        return redirect(url_for('manager_dashboard'))
    elif user['role'] == 3:
        return redirect(url_for('employee_dashboard'))
    
    session.clear()
    return redirect(url_for('login_page'))

@app.route('/admin/dashboard')
def admin_dashboard():
    user = current_user()
    if not user or user['role'] != 1:
        return redirect(url_for('login_page'))
    return render_template('admin_dashboard.html', user=user)

@app.route('/manager/dashboard')
def manager_dashboard():
    user = current_user()
    if not user or user['role'] != 2:
        return redirect(url_for('login_page'))
    return render_template('manager_dashboard.html', user=user)

@app.route('/employee/dashboard')
def employee_dashboard():
    user = current_user()
    if not user or user['role'] != 3:
        return redirect(url_for('login_page'))
    return render_template('employee_dashboard.html', user=user)

@app.route('/logout')
def logout():
    user = current_user()
    if user:
        log_activity(
            'Admin' if user['role'] == 1 else ('Manager' if user['role'] == 2 else 'Employee'),
            user['username'],
            'User Logout',
            'Logged out successfully.'
        )
    session.clear()
    return redirect(url_for('login_page'))

# ---------------------------------------------------------------------------
# API: Authentication & Password Recovery
# ---------------------------------------------------------------------------
@app.route('/api/login', methods=['POST'])
def api_login():
    data = request.get_json() or {}
    role_choice = int(data.get('role', 0)) # 1=Admin, 2=Manager, 3=Employee
    username = data.get('username', '').strip()
    password = data.get('password', '').strip()
    
    if not username or not password or role_choice not in [1, 2, 3]:
        return jsonify({'success': False, 'message': 'Username/ID, password, and role selection are required.'}), 400

    # 1 & 2: Admin or Manager login
    if role_choice in [1, 2]:
        attempts = get_failed_attempts(username)
        if attempts >= 3:
            return jsonify({
                'success': False,
                'lockout': True,
                'message': 'Maximum login attempts reached for this user.'
            })
            
        user = AdminManager.query.filter_by(username=username, role=role_choice).first()
        if user and verify_password(user.password_hash, password):
            reset_failed_attempts(username)
            session['user_id'] = user.username
            session['username'] = user.username
            session['role'] = user.role
            session['emp_id'] = 0
            
            log_activity('Admin' if user.role == 1 else 'Manager', user.username, 'Login Success', 'Logged into dashboard')
            return jsonify({'success': True, 'redirect': url_for('dashboard')})
        else:
            new_attempts = increase_failed_attempts(username)
            remaining = max(0, 3 - new_attempts)
            log_activity('System', username, 'Failed Login Attempt', f'Role: {role_choice}, Attempts: {new_attempts}')
            
            if new_attempts >= 3:
                return jsonify({
                    'success': False,
                    'lockout': True,
                    'message': 'Maximum login attempts reached.'
                })
            return jsonify({
                'success': False,
                'remaining': remaining,
                'message': f'Invalid Username or Password. Remaining attempts: {remaining}'
            })

    # 3: Employee login
    elif role_choice == 3:
        try:
            emp_id = int(username)
        except ValueError:
            return jsonify({'success': False, 'message': 'Employee ID must be a valid integer.'}), 400

        attempts = get_failed_attempts(emp_id)
        if attempts >= 3:
            return jsonify({
                'success': False,
                'lockout': True,
                'message': 'Maximum login attempts reached for this Employee ID.'
            })
            
        emp = Employee.query.get(emp_id)
        if emp and verify_password(emp.password_hash, password):
            reset_failed_attempts(emp_id)
            session['user_id'] = emp.id
            session['username'] = emp.name
            session['role'] = 3
            session['emp_id'] = emp.id
            
            log_activity('Employee', emp.name, 'Employee Login', f'ID: {emp.id}')
            return jsonify({'success': True, 'redirect': url_for('dashboard')})
        else:
            new_attempts = increase_failed_attempts(emp_id)
            remaining = max(0, 3 - new_attempts)
            log_activity('System', f"Emp-{emp_id}", 'Failed Login Attempt', f'Attempts: {new_attempts}')
            
            if new_attempts >= 3:
                return jsonify({
                    'success': False,
                    'lockout': True,
                    'message': 'Maximum login attempts reached.'
                })
            return jsonify({
                'success': False,
                'remaining': remaining,
                'message': f'Invalid Employee ID or Password. Remaining attempts: {remaining}'
            })

@app.route('/api/reset-password', methods=['POST'])
def api_reset_password():
    """
    Forgot Password workflow from C program logic:
    For Admin/Manager: verify email match
    For Employee: verify DOB match
    Then allow strong password creation.
    """
    data = request.get_json() or {}
    role_choice = int(data.get('role', 0))
    identifier = data.get('identifier', '').strip()
    verify_val = data.get('verify_value', '').strip() # email or DOB
    new_password = data.get('new_password', '').strip()

    is_strong, msg = create_strong_password(new_password)
    if not is_strong:
        return jsonify({'success': False, 'message': msg}), 400

    if role_choice in [1, 2]:
        user = AdminManager.query.filter_by(username=identifier, role=role_choice).first()
        if not user:
            user = AdminManager.query.filter_by(email=verify_val, role=role_choice).first()
        
        if user and user.email.lower() == verify_val.lower():
            user.password_hash = hash_password(new_password)
            reset_failed_attempts(user.username)
            db.session.commit()
            log_activity('Admin' if user.role == 1 else 'Manager', user.username, 'Password Reset', 'Password reset via email verification.')
            return jsonify({'success': True, 'message': 'Password reset successfully. You can now login.'})
        else:
            return jsonify({'success': False, 'message': 'Email verification failed or account not found.'}), 400

    elif role_choice == 3:
        try:
            emp_id = int(identifier)
        except ValueError:
            return jsonify({'success': False, 'message': 'Employee ID must be a valid integer.'}), 400

        emp = Employee.query.get(emp_id)
        if emp and emp.date_of_birth.strip() == verify_val.strip():
            emp.password_hash = hash_password(new_password)
            reset_failed_attempts(emp_id)
            db.session.commit()
            log_activity('Employee', emp.name, 'Password Reset', 'Password reset via DOB verification.')
            return jsonify({'success': True, 'message': 'Password reset successfully. You can now login.'})
        else:
            return jsonify({'success': False, 'message': 'Date of Birth does not match employee record.'}), 400

    return jsonify({'success': False, 'message': 'Invalid reset parameters.'}), 400

# ---------------------------------------------------------------------------
# API: Live Activity Feed (For Admin & Manager Real-Time Monitoring)
# ---------------------------------------------------------------------------
@app.route('/api/live-updates')
def api_live_updates():
    user = current_user()
    if not user or user['role'] not in [1, 2]:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    logs = ActivityLog.query.order_by(ActivityLog.timestamp.desc()).limit(15).all()
    return jsonify({
        'success': True,
        'logs': [l.to_dict() for l in logs]
    })

# ---------------------------------------------------------------------------
# API: Admin Features
# ---------------------------------------------------------------------------
@app.route('/api/admin/employees', methods=['GET'])
def api_admin_get_employees():
    user = current_user()
    if not user or user['role'] not in [1, 2]:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    employees = Employee.query.all()
    result = []
    for emp in employees:
        p_days = count_present_days(emp.id)
        result.append(emp.to_dict(present_days=p_days))

    return jsonify({'success': True, 'employees': result})

@app.route('/api/admin/employee/add', methods=['POST'])
def api_admin_add_employee():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    name = data.get('name', '').strip()
    email = data.get('email', '').strip()
    dob = data.get('date_of_birth', '').strip()
    position = data.get('position', '').strip()
    base_salary = float(data.get('base_salary', 0.0))
    raw_pwd = data.get('password', '').strip()

    if not name or not position:
        return jsonify({'success': False, 'message': 'Name and position are required.'}), 400

    if not is_valid_email(email):
        return jsonify({'success': False, 'message': 'Invalid email! Email must end with @gmail.com or @diu.edu.bd'}), 400

    is_strong, msg = create_strong_password(raw_pwd)
    if not is_strong:
        return jsonify({'success': False, 'message': msg}), 400

    # Auto-generate Employee ID starting 1001 (matching generateEmployeeId in admin.c)
    max_emp = db.session.query(db.func.max(Employee.id)).scalar()
    new_id = (max_emp + 1) if max_emp and max_emp >= 1000 else 1001

    new_emp = Employee(
        id=new_id,
        name=name,
        email=email,
        date_of_birth=dob,
        position=position,
        base_salary=base_salary,
        password_hash=hash_password(raw_pwd),
        bonus=0.0,
        deduction=0.0
    )
    db.session.add(new_emp)
    db.session.commit()

    log_activity('Admin', user['username'], 'Added Employee', f'Created employee #{new_id} ({name})')
    return jsonify({'success': True, 'message': f'Employee added successfully with ID #{new_id}', 'emp_id': new_id})

@app.route('/api/admin/employee/update', methods=['POST'])
def api_admin_update_employee():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    emp_id = int(data.get('id', 0))
    emp = Employee.query.get(emp_id)

    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    email = data.get('email', emp.email).strip()
    if email and not is_valid_email(email):
        return jsonify({'success': False, 'message': 'Invalid email format.'}), 400

    emp.name = data.get('name', emp.name).strip()
    emp.email = email
    emp.date_of_birth = data.get('date_of_birth', emp.date_of_birth).strip()
    emp.position = data.get('position', emp.position).strip()
    emp.base_salary = float(data.get('base_salary', emp.base_salary))

    db.session.commit()
    log_activity('Admin', user['username'], 'Updated Employee', f'Updated records for employee #{emp_id}')
    return jsonify({'success': True, 'message': 'Employee details updated successfully.'})

@app.route('/api/admin/employee/delete', methods=['POST'])
def api_admin_delete_employee():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    emp_id = int(data.get('id', 0))
    emp = Employee.query.get(emp_id)

    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    # Delete employee attendance history (matching deleteEmployeeAttendance in admin.c)
    Attendance.query.filter_by(emp_id=emp_id).delete()
    Leave.query.filter_by(emp_id=emp_id).delete()
    db.session.delete(emp)
    db.session.commit()

    log_activity('Admin', user['username'], 'Deleted Employee', f'Deleted employee #{emp_id} ({emp.name}) and attendance history.')
    return jsonify({'success': True, 'message': 'Employee deleted successfully.'})

@app.route('/api/admin/employee/role', methods=['POST'])
def api_admin_manage_role():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    emp_id = int(data.get('id', 0))
    new_position = data.get('position', '').strip()

    emp = Employee.query.get(emp_id)
    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    old_pos = emp.position
    emp.position = new_position
    db.session.commit()

    log_activity('Admin', user['username'], 'Changed Job Role', f'Changed #{emp_id} position from {old_pos} to {new_position}')
    return jsonify({'success': True, 'message': 'Job role updated successfully.'})

@app.route('/api/admin/attendance/date', methods=['GET'])
def api_admin_get_attendance_by_date():
    user = current_user()
    if not user or user['role'] not in [1, 2]:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    date_str = request.args.get('date', datetime.utcnow().strftime('%d/%m/%Y')).strip()
    employees = Employee.query.all()
    
    # Query attendance records for this date
    att_map = {att.emp_id: att.status for att in Attendance.query.filter_by(date=date_str).all()}

    results = []
    for emp in employees:
        status = att_map.get(emp.id, None) # 1=Present, 0=Absent, None=Not recorded
        results.append({
            'id': emp.id,
            'name': emp.name,
            'position': emp.position,
            'status': status
        })

    return jsonify({
        'success': True,
        'date': date_str,
        'employees': results
    })

@app.route('/api/admin/attendance/batch', methods=['POST'])
def api_admin_batch_attendance():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    date_str = data.get('date', datetime.utcnow().strftime('%d/%m/%Y')).strip()
    records = data.get('records', []) # list of { emp_id, status }

    if not records:
        return jsonify({'success': False, 'message': 'No attendance records submitted.'}), 400

    updated_count = 0
    for r in records:
        emp_id = int(r.get('emp_id', 0))
        status = int(r.get('status', 1)) # 1=Present, 0=Absent
        
        emp = Employee.query.get(emp_id)
        if not emp:
            continue

        existing = Attendance.query.filter_by(emp_id=emp_id, date=date_str).first()
        if existing:
            existing.status = status
        else:
            att = Attendance(emp_id=emp_id, date=date_str, status=status)
            db.session.add(att)
        updated_count += 1

    db.session.commit()
    log_activity('Admin', user['username'], 'Batch Attendance Marked', f'Recorded attendance for {updated_count} employees on {date_str}')
    return jsonify({
        'success': True,
        'message': f'Attendance successfully saved for {updated_count} employees on {date_str}.'
    })

@app.route('/api/admin/attendance/record', methods=['POST'])
def api_admin_record_attendance():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    emp_id = int(data.get('emp_id', 0))
    status = int(data.get('status', 1)) # 1=Present, 0=Absent
    date_str = data.get('date', datetime.utcnow().strftime('%d/%m/%Y')).strip()

    emp = Employee.query.get(emp_id)
    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    existing = Attendance.query.filter_by(emp_id=emp_id, date=date_str).first()
    if existing:
        existing.status = status
        db.session.commit()
        log_activity('Admin', user['username'], 'Updated Attendance', f'Updated #{emp_id} attendance for {date_str}')
        return jsonify({'success': True, 'message': f'Attendance updated for {emp.name} on {date_str}.'})
    else:
        att = Attendance(emp_id=emp_id, date=date_str, status=status)
        db.session.add(att)
        db.session.commit()
        log_activity('Admin', user['username'], 'Recorded Attendance', f'Recorded #{emp_id} ({emp.name}) as {"Present" if status == 1 else "Absent"} on {date_str}')
        return jsonify({'success': True, 'message': f'Attendance recorded for {emp.name}.'})


@app.route('/api/admin/payroll/bonus', methods=['POST'])
def api_admin_add_bonus():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    emp_id = int(data.get('emp_id', 0))
    bonus_val = float(data.get('bonus', 0.0))

    if bonus_val < 0:
        return jsonify({'success': False, 'message': 'Bonus amount cannot be negative.'}), 400

    emp = Employee.query.get(emp_id)
    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    emp.bonus += bonus_val
    db.session.commit()

    log_activity('Admin', user['username'], 'Added Bonus', f'Added ${bonus_val:.2f} bonus to #{emp_id} ({emp.name})')
    return jsonify({'success': True, 'message': f'Bonus of ${bonus_val:.2f} added successfully.'})

@app.route('/api/admin/payroll/deduction', methods=['POST'])
def api_admin_add_deduction():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    emp_id = int(data.get('emp_id', 0))
    ded_val = float(data.get('deduction', 0.0))

    if ded_val < 0:
        return jsonify({'success': False, 'message': 'Deduction amount cannot be negative.'}), 400

    emp = Employee.query.get(emp_id)
    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    emp.deduction += ded_val
    db.session.commit()

    log_activity('Admin', user['username'], 'Added Deduction', f'Added ${ded_val:.2f} deduction to #{emp_id} ({emp.name})')
    return jsonify({'success': True, 'message': f'Deduction of ${ded_val:.2f} added successfully.'})

@app.route('/api/admin/users/add', methods=['POST'])
def api_admin_add_system_user():
    user = current_user()
    if not user or user['role'] != 1:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    new_username = data.get('username', '').strip()
    new_email = data.get('email', f"{new_username}@gmail.com").strip()
    raw_pwd = data.get('password', '').strip()
    new_role = int(data.get('role', 2)) # 1=Admin, 2=Manager

    if not new_username or not raw_pwd:
        return jsonify({'success': False, 'message': 'Username and password are required.'}), 400

    if AdminManager.query.get(new_username):
        return jsonify({'success': False, 'message': 'Username already exists.'}), 400

    is_strong, msg = create_strong_password(raw_pwd)
    if not is_strong:
        return jsonify({'success': False, 'message': msg}), 400

    new_user = AdminManager(
        username=new_username,
        email=new_email,
        password_hash=hash_password(raw_pwd),
        role=new_role
    )
    db.session.add(new_user)
    db.session.commit()

    role_name = 'Admin' if new_role == 1 else 'Manager'
    log_activity('Admin', user['username'], 'Created User Account', f'Created new {role_name} account: {new_username}')
    return jsonify({'success': True, 'message': f'{role_name} account "{new_username}" created successfully.'})

# ---------------------------------------------------------------------------
# API: Manager Features & Leave Approvals
# ---------------------------------------------------------------------------
@app.route('/api/manager/leaves/pending', methods=['GET'])
def api_manager_pending_leaves():
    user = current_user()
    if not user or user['role'] not in [1, 2]:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    leaves = Leave.query.order_by(Leave.id.desc()).all()
    results = []
    for l in leaves:
        emp = Employee.query.get(l.emp_id)
        d = l.to_dict()
        d['employee_name'] = emp.name if emp else f"ID #{l.emp_id}"
        results.append(d)

    return jsonify({'success': True, 'leaves': results})

@app.route('/api/manager/leaves/approve', methods=['POST'])
def api_manager_approve_leave():
    user = current_user()
    if not user or user['role'] not in [1, 2]:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    leave_id = int(data.get('leave_id', 0))
    action = int(data.get('action', 1)) # 1 = Approve, 2 = Reject

    leave = Leave.query.get(leave_id)
    if not leave:
        return jsonify({'success': False, 'message': 'Leave request not found.'}), 404

    leave.status = action
    db.session.commit()

    status_txt = 'Approved' if action == 1 else 'Rejected'
    log_activity('Manager' if user['role'] == 2 else 'Admin', user['username'], f'Leave {status_txt}', f'{status_txt} leave request #{leave_id} for Employee #{leave.emp_id}')
    return jsonify({'success': True, 'message': f'Leave request #{leave_id} has been {status_txt}.'})

# ---------------------------------------------------------------------------
# API: Employee Features & Modern Interactive Attendance
# ---------------------------------------------------------------------------
@app.route('/api/employee/profile', methods=['GET'])
def api_employee_profile():
    user = current_user()
    if not user or user['role'] != 3:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    emp = Employee.query.get(user['emp_id'])
    if not emp:
        return jsonify({'success': False, 'message': 'Profile not found.'}), 404

    p_days = count_present_days(emp.id)
    return jsonify({'success': True, 'profile': emp.to_dict(present_days=p_days)})

@app.route('/api/employee/attendance', methods=['GET'])
def api_employee_attendance():
    user = current_user()
    if not user or user['role'] != 3:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    emp_id = user['emp_id']
    records = Attendance.query.filter_by(emp_id=emp_id).order_by(Attendance.id.desc()).all()
    
    today_str = datetime.utcnow().strftime('%d/%m/%Y')
    checked_in_today = any(r.date == today_str and r.status == 1 for r in records)
    
    # Calculate attendance streak
    streak = 0
    for r in records:
        if r.status == 1:
            streak += 1
        else:
            break

    total_present = count_present_days(emp_id)
    total_records = len(records)

    return jsonify({
        'success': True,
        'records': [r.to_dict() for r in records],
        'checked_in_today': checked_in_today,
        'today_date': today_str,
        'streak': streak,
        'total_present': total_present,
        'total_records': total_records
    })

@app.route('/api/employee/attendance/checkin', methods=['POST'])
def api_employee_checkin():
    user = current_user()
    if not user or user['role'] != 3:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    emp_id = user['emp_id']
    emp = Employee.query.get(emp_id)
    today_str = datetime.utcnow().strftime('%d/%m/%Y')

    existing = Attendance.query.filter_by(emp_id=emp_id, date=today_str).first()
    if existing:
        if existing.status == 1:
            return jsonify({'success': False, 'message': 'You have already checked in for today!'}), 400
        else:
            existing.status = 1
            db.session.commit()
    else:
        att = Attendance(emp_id=emp_id, date=today_str, status=1)
        db.session.add(att)
        db.session.commit()

    log_activity('Employee', emp.name, 'Attendance Check-in', f'Checked in for today ({today_str})')
    return jsonify({
        'success': True,
        'message': f'Check-in successful! Welcome, {emp.name}.',
        'today_date': today_str
    })

@app.route('/api/employee/leave/apply', methods=['POST'])
def api_employee_apply_leave():
    user = current_user()
    if not user or user['role'] != 3:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    from_date = data.get('from_date', '').strip()
    to_date = data.get('to_date', '').strip()
    reason = data.get('reason', '').strip()

    if not from_date or not to_date or not reason:
        return jsonify({'success': False, 'message': 'From Date, To Date, and Reason are required.'}), 400

    if '|' in reason:
        return jsonify({'success': False, 'message': "Reason cannot contain '|' character."}), 400

    apply_date = datetime.utcnow().strftime('%d/%m/%Y')

    leave = Leave(
        emp_id=user['emp_id'],
        apply_date=apply_date,
        from_date=from_date,
        to_date=to_date,
        reason=reason,
        status=0 # Pending
    )
    db.session.add(leave)
    db.session.commit()

    log_activity('Employee', user['username'], 'Applied For Leave', f'From {from_date} to {to_date} ({reason[:30]}...)')
    return jsonify({'success': True, 'message': 'Leave application submitted successfully. Status: Pending.'})

@app.route('/api/employee/leave/status', methods=['GET'])
def api_employee_leave_status():
    user = current_user()
    if not user or user['role'] != 3:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    leaves = Leave.query.filter_by(emp_id=user['emp_id']).order_by(Leave.id.desc()).all()
    return jsonify({'success': True, 'leaves': [l.to_dict() for l in leaves]})

@app.route('/api/employee/profile/update', methods=['POST'])
def api_employee_update_profile():
    user = current_user()
    if not user or user['role'] != 3:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    emp = Employee.query.get(user['emp_id'])
    if not emp:
        return jsonify({'success': False, 'message': 'Employee not found.'}), 404

    data = request.get_json() or {}
    new_name = data.get('name', emp.name).strip()
    new_email = data.get('email', emp.email).strip()
    new_dob = data.get('date_of_birth', emp.date_of_birth).strip()

    if new_email and not is_valid_email(new_email):
        return jsonify({'success': False, 'message': 'Invalid email! Email must end with @gmail.com or @diu.edu.bd'}), 400

    emp.name = new_name
    emp.email = new_email
    emp.date_of_birth = new_dob
    db.session.commit()

    log_activity('Employee', emp.name, 'Updated Profile', 'Updated personal profile details.')
    return jsonify({'success': True, 'message': 'Profile updated successfully.'})

@app.route('/api/change-password', methods=['POST'])
def api_change_password():
    user = current_user()
    if not user:
        return jsonify({'success': False, 'message': 'Unauthorized'}), 403

    data = request.get_json() or {}
    current_pass = data.get('current_password', '').strip()
    new_pass = data.get('new_password', '').strip()

    is_strong, msg = create_strong_password(new_pass)
    if not is_strong:
        return jsonify({'success': False, 'message': msg}), 400

    # 1 & 2: Admin or Manager
    if user['role'] in [1, 2]:
        adm = AdminManager.query.get(user['username'])
        if not adm or not verify_password(adm.password_hash, current_pass):
            return jsonify({'success': False, 'message': 'Current password is incorrect.'}), 400

        adm.password_hash = hash_password(new_pass)
        db.session.commit()
        log_activity('Admin' if user['role'] == 1 else 'Manager', user['username'], 'Changed Password', 'Successfully changed account password.')
        return jsonify({'success': True, 'message': 'Password changed successfully.'})

    # 3: Employee
    elif user['role'] == 3:
        emp = Employee.query.get(user['emp_id'])
        if not emp or not verify_password(emp.password_hash, current_pass):
            return jsonify({'success': False, 'message': 'Current password is incorrect.'}), 400

        emp.password_hash = hash_password(new_pass)
        db.session.commit()
        log_activity('Employee', emp.name, 'Changed Password', 'Successfully changed account password.')
        return jsonify({'success': True, 'message': 'Password changed successfully.'})

    return jsonify({'success': False, 'message': 'Invalid request.'}), 400

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port, debug=True)
