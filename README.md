# DroneMap_HW

### C++ Component (main.cpp)
The `main.cpp` file generates objects and stores relevant data in an SQLite database. To build and run:

`g++ main.cpp -o main -lsqlite3 && ./main`

**Dependencies:** SQLite might not work as is. For bulding your own follow the instructions [here](https://www.tutorialspoint.com/sqlite/sqlite_installation.htm).

### Python Component (API_app.py)
The `API_app.py` serves as an API app that handles API calls. Run it with:

`python3 API_app.py`

Sample API call: `http://127.0.0.1:5000/api/section/A/1164978000000/1164988300000`

**Dependencies:** Install required Python packages:

`pip install Flask db-sqlite3 matplotlib`

---
