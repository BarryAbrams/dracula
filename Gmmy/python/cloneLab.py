from flask import Flask, render_template, jsonify, request
import json, os

app = Flask(__name__)

class CueManager:
    def __init__(self):
        self.json_dir = './json'
        self.cues_file = os.path.join(self.json_dir, 'cues.json')
        self.cues = self.load_json(self.cues_file)

    def load_json(self, filepath):
        with open(filepath, 'r') as f:
            return json.load(f)

    def save_json(self, data, filepath):
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=4)

    def get_all_cues(self):
        return self.cues

    def get_cue(self, cue_id):
        return next((cue for cue in self.cues if cue['id'] == cue_id), None)

    def create_cue(self, cue_data):
        # Assuming cues do not have an 'id' and it needs to be assigned
        new_id = max((cue['id'] for cue in self.cues), default=0) + 1
        cue_data['id'] = new_id
        self.cues.append(cue_data)
        self.save_json(self.cues, self.cues_file)
        return cue_data

    def update_cue(self, cue_id, update_data):
        cue = self.get_cue(cue_id)
        if cue:
            cue_index = self.cues.index(cue)
            self.cues[cue_index].update(update_data)
            self.save_json(self.cues, self.cues_file)
            return self.cues[cue_index]
        return None

    def delete_cue(self, cue_id):
        cue = self.get_cue(cue_id)
        if cue:
            self.cues.remove(cue)
            self.save_json(self.cues, self.cues_file)
            return True
        return False

    def add_action_to_cue(self, cue_id, action_data):
        cue = self.get_cue(cue_id)
        if cue:
            if 'actions' not in cue:
                cue['actions'] = []
            new_action_id = max((action['id'] for action in cue['actions']), default=0) + 1
            action_data['id'] = new_action_id
            cue['actions'].append(action_data)
            self.save_json(self.cues, self.cues_file)
            return action_data
        return None

    def update_action(self, cue_id, action_id, update_data):
        cue = self.get_cue(cue_id)
        if cue and 'actions' in cue:
            action = next((action for action in cue['actions'] if action['id'] == action_id), None)
            if action:
                action_index = cue['actions'].index(action)
                cue['actions'][action_index].update(update_data)
                self.save_json(self.cues, self.cues_file)
                return cue['actions'][action_index]
        return None

    def remove_action_from_cue(self, cue_id, action_id):
        cue = self.get_cue(cue_id)
        if cue and 'actions' in cue:
            action = next((action for action in cue['actions'] if action['id'] == action_id), None)
            if action:
                cue['actions'].remove(action)
                self.save_json(self.cues, self.cues_file)
                return True
        return False



cue_manager = CueManager()

@app.route('/cues/interface')
def cues_interface():
    return render_template('cues_interface.html', cues=cue_manager.get_all_cues())

@app.route('/cues', methods=['GET', 'POST'])
def cues():
    if request.method == 'GET':
        return jsonify(cue_manager.get_all_cues())
    elif request.method == 'POST':
        data = request.json
        return jsonify(cue_manager.create_cue(data))

@app.route('/cues/<int:cue_id>', methods=['GET', 'PUT', 'DELETE'])
def cue(cue_id):
    if request.method == 'GET':
        return jsonify(cue_manager.get_cue(cue_id))
    elif request.method == 'PUT':
        data = request.json
        return jsonify(cue_manager.update_cue(cue_id, data))
    elif request.method == 'DELETE':
        return jsonify(cue_manager.delete_cue(cue_id))

@app.route('/cues/<int:cue_id>/actions', methods=['POST'])
def add_action_to_cue(cue_id):
    data = request.json
    return jsonify(cue_manager.add_action_to_cue(cue_id, data))

@app.route('/cues/<int:cue_id>/actions/<int:action_id>', methods=['PUT', 'DELETE'])
def action(cue_id, action_id):
    if request.method == 'PUT':
        data = request.json
        return jsonify(cue_manager.update_action(cue_id, action_id, data))
    elif request.method == 'DELETE':
        return jsonify(cue_manager.remove_action_from_cue(cue_id, action_id))

def send_sound_to_arduino(sound_id):
    message = f"{sound_id}"
    ser.write(message.encode())  # Assuming ser is your serial connection

def send_light_to_arduino(fixtures, animation_id):
    # Translate fixtures and animation into a message for Arduino
    # For example, it might look like "1,255,128,64;2,255,128,64"
    message = f"{fixtures},{animation_id}"
    ser.write(message.encode())



if __name__ == '__main__':
    app.run(debug=True, port=5001)
