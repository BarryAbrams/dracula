from gtts import gTTS
import pygame
import os

# Create a directory to store the audio files (if it doesn't exist)
output_base_dir = "audio_files"
os.makedirs(output_base_dir, exist_ok=True)

# Generate 120 audio files, 15 at a time, in separate folders
for folder_num in range(1, 9):  # Create 8 folders
    folder_name = f"INPUT{folder_num}"
    folder_path = os.path.join(output_base_dir, folder_name)
    os.makedirs(folder_path, exist_ok=True)

    for file_num in range(1, 16):  # Create 15 files per folder
        file_name = f"{file_num:03d}.mp3"  # Change the file extension to .mp3
        file_path = os.path.join(folder_path, file_name)

        # Convert the number to text
        text = str((folder_num - 1) * 15 + file_num)

        # Create a gTTS object
        tts = gTTS(text, lang="en")

        # Save the audio as an MP3 file
        tts.save(file_path)

        print(f"Generated audio file {os.path.join(folder_name, file_name)}")

# Initialize pygame mixer (needed for MP3 file handling)
pygame.mixer.init()

# Play one of the generated audio files (e.g., the first one)
audio_file_to_play = os.path.join(output_base_dir, "INPUT1", "001.mp3")
pygame.mixer.music.load(audio_file_to_play)
pygame.mixer.music.play()

# Wait for the audio to finish playing
while pygame.mixer.music.get_busy():
    continue