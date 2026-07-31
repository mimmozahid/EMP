import sys
import os

# Add parent directory to path so imports work seamlessly on Vercel
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from app import app

# Vercel Serverless Function entry point
app = app
