from flask import Flask, request, jsonify
from flask_cors import CORS

import sqlite3
import matplotlib
import matplotlib.pyplot as plt
matplotlib.use('Agg')

app = Flask(__name__)
CORS(app, resources={r"/*": {"origins": "*"}})

db_name = "drone_map.db"

def connect_to_db(name):
    conn = sqlite3.connect(name)
    return conn

def get_trajectory(id, start_time, end_time):
    trajectory_data = []

    try:
        with sqlite3.connect(db_name) as conn:
            conn.row_factory = sqlite3.Row
            cur = conn.cursor()

            table_name = f"{id}_trajectory"
            query = f"SELECT * FROM {table_name} WHERE time BETWEEN ? AND ?;"

            cur.execute(query, (start_time, end_time))
            rows = cur.fetchall()

            for row in rows:
                data_entry = {
                    "time": row["time"],
                    "x": row["x"],
                    "y": row["y"],
                    "heading": row["heading"]
                }
                trajectory_data.append(data_entry)

    except Exception as e:
        print("Error fetching trajectory data:")
        print(e)

    return trajectory_data


def get_object_constants(id, no_payload = False):
    data_entry = {}

    try:
        with sqlite3.connect(db_name) as conn:
            conn.row_factory = sqlite3.Row
            cur = conn.cursor()

            table_name = f"{id}_data"
            query = f"SELECT * FROM {table_name};"

            cur.execute(query)
            rows = cur.fetchall()

            assert len(rows) == 1

            row = rows[0]
            if (no_payload):
                data_entry = {
                    "speed": row["speed"],
                    "created_time": row["created_time"],
                    "expire_time": row["expire_time"]
                }
            else:
                data_entry = {
                    "speed": row["speed"],
                    "created_time": row["created_time"],
                    "expire_time": row["expire_time"],
                    "payload": row["payload"]
                }


    except Exception as e:
        print("Error fetching data:")
        print(e)
        data_entry = {}

    return data_entry

def get_object_data(object_id, start_time, end_time):
    output_json = {
        "constants": get_object_constants(object_id),
        "trajectory": get_trajectory(object_id, start_time, end_time)
    }

    return output_json

def get_section_traffic(table_name):
    data_list = []

    try:
        with sqlite3.connect(db_name) as conn:
            conn.row_factory = sqlite3.Row
            cur = conn.cursor()

            query = f"SELECT * FROM {table_name};"
            cur.execute(query)
            rows = cur.fetchall()

            for row in rows:
                data_entry = {
                    "id_index": row["id_index"],
                    "start": row["start"],
                    "end": row["end"]
                }
                data_list.append(data_entry)

    except Exception as e:
        print(f"Error fetching data from {table_name}:")
        print(e)

    return data_list

def construct_trajectories(section_id, start_time, end_time):
    section_traffic = get_section_traffic(section_id)
    trajectories = {}

    for data in section_traffic:
        object_id = data['id_index'][:-2]  # remove index from id
        min_start_time = max(int(start_time), int(data['start']))
        max_end_time = min(int(end_time), int(data['end']))

        path = get_trajectory(object_id, min_start_time, max_end_time)

        if object_id in trajectories:
            if path and path[0]['time'] > trajectories[object_id]['trajectory'][0]['time']: # This is necessary to add trajectory in chronological order
                trajectories[object_id]['trajectory'] = trajectories[object_id]['trajectory'] + path
            else:
                trajectories[object_id]['trajectory'] = path + trajectories[object_id]['trajectory']
        else:
            if path:
                trajectories[object_id] = {
                    'trajectory': path,
                    'constants': get_object_constants(object_id, True),
                }

    sorted_trajectories = dict(sorted(trajectories.items(), key=lambda item: item[1]['constants']['created_time'], reverse=True))

    return sorted_trajectories

#Set up API endpoints
@app.route('/api/trajectory/<user_id>/<start_time>/<end_time>', methods=['GET'])
def api_get_object(user_id, start_time, end_time):
    return jsonify(get_object_data(user_id, start_time, end_time))

@app.route('/api/section/<section_id>/<start_time>/<end_time>', methods=['GET'])
def api_get_section(section_id, start_time, end_time):
    return jsonify(construct_trajectories(section_id, start_time, end_time))

#Plotting utils
def plot_section_trajectories(trajectories):
    plt.figure(figsize=(8, 8))

    for object_id, trajectory in trajectories.items():
        x_values = [point['x'] for point in trajectory['trajectory']]
        y_values = [point['y'] for point in trajectory['trajectory']]
        plt.plot(x_values, y_values, label=object_id)

    plt.title('Trajectories')
    plt.xlabel('X-coordinate')
    plt.ylabel('Y-coordinate')
    plt.grid(True)
    plt.axis([0, 1000000, 0, 1000000])

    plt.savefig('trajectories_section_plot.png')  # Save the plot to a file instead of displaying it. (needed because I'm using WSL for this task)
    plt.close()

def plot_single_trajectory(trajectory, object_id):
    plt.figure(figsize=(8, 8))

    x_values = [point['x'] for point in trajectory]
    y_values = [point['y'] for point in trajectory]

    plt.plot(x_values, y_values, label=object_id)

    plt.title(f'Trajectory for Object {object_id}')
    plt.xlabel('X-coordinate')
    plt.ylabel('Y-coordinate')
    plt.grid(True)
    plt.axis([0, 1000000, 0, 1000000])

    plt.savefig('trajectory.png')
    plt.close()


if __name__ == "__main__":
    app.run()

    # # Uncomment to get quick visualization (comment app.run())
    # id = 'drt2i7l0tloc0r1dxlx23993cb1uwxdn' # get this from generator (main.cpp) output or from an api call
    # start_time = 1164978000000 # time is in milliseconds
    # end_time = start_time + 1000 * 3600 * 10 # 10h of data

    # plot_section_trajectories(construct_trajectories('C', start_time, end_time))

    # # plot_single_trajectory(get_object_data(id, start_time, end_time)['trajectory'], id)
