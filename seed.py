import os
from models import db, AdminManager, Employee, Attendance, ActivityLog
from auth import hash_password, log_activity

def seed_database(app):
    with app.app_context():
        db.create_all()
        
        # 1. Seed AdminManager accounts if empty
        if AdminManager.query.count() == 0:
            csv_path = os.path.join(app.root_path, 'AdminManager.csv')
            if os.path.exists(csv_path):
                with open(csv_path, 'r', encoding='utf-8') as f:
                    for line in f:
                        line = line.strip()
                        if not line:
                            continue
                        parts = line.split('|')
                        if len(parts) >= 4:
                            username = parts[0]
                            email = parts[1]
                            pwd_hash = parts[2]
                            role = int(parts[3])
                            user = AdminManager(username=username, email=email, password_hash=pwd_hash, role=role)
                            db.session.add(user)
                        elif len(parts) >= 3:
                            username = parts[0]
                            pwd_hash = parts[1]
                            role = int(parts[2])
                            email = f"{username}@gmail.com"
                            user = AdminManager(username=username, email=email, password_hash=pwd_hash, role=role)
                            db.session.add(user)
            
            # Default accounts if CSV didn't contain them
            if not AdminManager.query.filter_by(username='admin').first():
                admin_user = AdminManager(
                    username='admin',
                    email='admin@gmail.com',
                    password_hash=hash_password('admin123'),
                    role=1
                )
                db.session.add(admin_user)
                
            if not AdminManager.query.filter_by(username='manager').first():
                manager_user = AdminManager(
                    username='manager',
                    email='manager@gmail.com',
                    password_hash=hash_password('manager123'),
                    role=2
                )
                db.session.add(manager_user)
            
            db.session.commit()
            print("AdminManager accounts seeded.")

        # 2. Seed Employee accounts if empty
        if Employee.query.count() == 0:
            emp_csv_path = os.path.join(app.root_path, 'employees.csv')
            if os.path.exists(emp_csv_path):
                with open(emp_csv_path, 'r', encoding='utf-8') as f:
                    for line in f:
                        line = line.strip()
                        if not line:
                            continue
                        parts = line.split('|')
                        if len(parts) >= 9:
                            emp = Employee(
                                id=int(parts[0]),
                                name=parts[1],
                                email=parts[2],
                                date_of_birth=parts[3],
                                position=parts[4],
                                base_salary=float(parts[5]),
                                password_hash=parts[6],
                                bonus=float(parts[7]),
                                deduction=float(parts[8])
                            )
                            db.session.add(emp)
                db.session.commit()
                print("Employees seeded from CSV.")
            
            # Seed default employee if table is still empty
            if Employee.query.count() == 0:
                default_emp = Employee(
                    id=1001,
                    name='mim',
                    email='mim@gmail.com',
                    date_of_birth='12/01/2003',
                    position='Software Engineer',
                    base_salary=40000.0,
                    password_hash=hash_password('mim123'),
                    bonus=0.0,
                    deduction=0.0
                )
                db.session.add(default_emp)
                db.session.commit()
                print("Default employee 1001 seeded.")

        # 3. Seed Attendance if empty
        if Attendance.query.count() == 0:
            att_csv_path = os.path.join(app.root_path, 'attendance.csv')
            if os.path.exists(att_csv_path):
                with open(att_csv_path, 'r', encoding='utf-8') as f:
                    for line in f:
                        line = line.strip()
                        if not line:
                            continue
                        parts = line.split('|')
                        if len(parts) >= 3:
                            att = Attendance(
                                emp_id=int(parts[0]),
                                date=parts[1],
                                status=int(parts[2])
                            )
                            db.session.add(att)
                db.session.commit()

        # Add initial activity log if none exist
        if ActivityLog.query.count() == 0:
            log_activity('System', 'SystemInit', 'System Initialized', 'Database seeded with default records.')
