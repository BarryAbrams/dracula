import re, os, subprocess

def extract_ogg_file_names(js_code):
    # Pattern to match .ogg file names
    pattern = r'[\w-]+\.ogg'
    # Find all matches
    file_names = re.findall(pattern, js_code)
    return file_names

def download_files(base_url, file_names, directory):
    # Create the directory if it does not exist
    if not os.path.exists(directory):
        os.makedirs(directory)

    for file_name in file_names:
        full_url = base_url + file_name
        # Modify the curl command to save the file in the specified directory
        curl_command = f"curl -o {directory}/{file_name} {full_url}"
        os.system(curl_command)
        print(f"Downloaded: {full_url}")

def convert_to_mp3(ogg_directory, mp3_directory):
    # Create the MP3 directory if it does not exist
    if not os.path.exists(mp3_directory):
        os.makedirs(mp3_directory)

    for file in os.listdir(ogg_directory):
        if file.endswith(".ogg"):
            ogg_file = os.path.join(ogg_directory, file)
            mp3_file = os.path.join(mp3_directory, file.replace(".ogg", ".mp3"))
            ffmpeg_command = f"ffmpeg -i {ogg_file} -acodec libmp3lame {mp3_file}"
            subprocess.run(ffmpeg_command.split())
            print(f"Converted {ogg_file} to {mp3_file}")

root = "dungeon"

# Base URL
base_url = "https://sounds.tabletopaudio.com/"+root+"/"

# Directory to save files
ogg_directory = "tt_audio/"+root+"/ogg"
mp3_directory = "tt_audio/"+root+"/mp3"

# Example JavaScript code block
js_code = """
function init(){if(!createjs.Sound.initializeDefaultPlugins())return document.getElementById("error").style.display="block",void(document.getElementById("content").style.display="none");var e="https://sounds.tabletopaudio.com/dungeon2/",a="https://sounds.tabletopaudio.com/combat2/",t="https://sounds.tabletopaudio.com/atlantis/",t=[{src:e+"dungeon_bg1.ogg",id:"bg_1",data:1},{src:e+"dungeon_bg2.ogg",id:"bg_2",data:1},{src:e+"drips1_lp.ogg",id:"bg_3",data:1},{src:e+"rumble.ogg",id:"bg_4",data:1},{src:e+"bats.ogg",id:"bg_5",data:1},{src:e+"door_open.ogg",id:"bg_6",data:1},{src:e+"torches.ogg",id:"bg_7",data:1},{src:e+"door_close.ogg",id:"bg_8",data:1},{src:e+"scrape1a.ogg",id:"bg_9",data:1},{src:e+"growl1.ogg",id:"bg_10",data:1},{src:e+"low_tone1.ogg",id:"bg_11",data:1},{src:e+"flies.ogg",id:"bg_12",data:1},{src:e+"hi_tone_1.ogg",id:"bg_13",data:1},{src:e+"creature1.ogg",id:"bg_14",data:1},{src:e+"hi_tone_2.ogg",id:"bg_15",data:1},{src:e+"music_1.ogg",id:"bg_16",data:1},{src:e+"combat1.ogg",id:"bg_17",data:1},{src:e+"footsteps.ogg",id:"bg_18",data:1},{src:e+"screams.ogg",id:"bg_19",data:1},{src:e+"rats.ogg",id:"bg_20",data:1},{src:e+"stream.ogg",id:"bg_21",data:1},{src:e+"tone3.ogg",id:"bg_22",data:1},{src:e+"drums2.ogg",id:"bg_23",data:1},{src:e+"bubbles.ogg",id:"bg_24",data:1},{src:e+"music_3a.ogg",id:"bg_25",data:1},{src:e+"steam_release.ogg",id:"bg_26",data:1},{src:e+"ooze.ogg",id:"bg_27",data:1},{src:e+"roar.ogg",id:"bg_28",data:1},{src:a+"trap_arrow.ogg",id:"bg_29",data:1},{src:a+"trap_pit.ogg",id:"bg_30",data:1},{src:a+"trap_spike.ogg",id:"bg_31",data:1},{src:a+"trap_gate.ogg",id:"bg_32",data:1},{src:a+"trap_poison.ogg",id:"bg_33",data:1},{src:a+"magic_trap.ogg",id:"bg_34",data:1},{src:t+"Atlantis-net_trap.ogg",id:"bg_35",data:1},{src:t+"Atlantis-blade_trap.ogg",id:"bg_36",data:1}];createjs.Sound.alternateExtensions=["mp3"],queue=new createjs.LoadQueue,queue.installPlugin(createjs.Sound),queue.addEventListener("complete",handleComplete),queue.loadManifest(t),createSound(),queue.on("progress",handleOverallProgress),doChecks(),whichPad="Dungeon_";for(var n=0,g=localStorage.length;n<g;++n)key=window.localStorage.key(n),0<=key.indexOf("SP_Dungeon")&&preLoad(key);TabletopAudioServer.init()}function createSound(){for(var e in mySounds={},mySounds.bg_1=createjs.Sound.createInstance("bg_1"),mySounds.bg_2=createjs.Sound.createInstance("bg_2"),mySounds.bg_3=createjs.Sound.createInstance("bg_3"),mySounds.bg_4=createjs.Sound.createInstance("bg_4"),mySounds.bg_5=createjs.Sound.createInstance("bg_5"),mySounds.bg_6=createjs.Sound.createInstance("bg_6"),mySounds.bg_7=createjs.Sound.createInstance("bg_7"),mySounds.bg_8=createjs.Sound.createInstance("bg_8"),mySounds.bg_9=createjs.Sound.createInstance("bg_9"),mySounds.bg_10=createjs.Sound.createInstance("bg_10"),mySounds.bg_11=createjs.Sound.createInstance("bg_11"),mySounds.bg_12=createjs.Sound.createInstance("bg_12"),mySounds.bg_13=createjs.Sound.createInstance("bg_13"),mySounds.bg_14=createjs.Sound.createInstance("bg_14"),mySounds.bg_15=createjs.Sound.createInstance("bg_15"),mySounds.bg_16=createjs.Sound.createInstance("bg_16"),mySounds.bg_17=createjs.Sound.createInstance("bg_17"),mySounds.bg_18=createjs.Sound.createInstance("bg_18"),mySounds.bg_19=createjs.Sound.createInstance("bg_19"),mySounds.bg_20=createjs.Sound.createInstance("bg_20"),mySounds.bg_21=createjs.Sound.createInstance("bg_21"),mySounds.bg_22=createjs.Sound.createInstance("bg_22"),mySounds.bg_23=createjs.Sound.createInstance("bg_23"),mySounds.bg_24=createjs.Sound.createInstance("bg_24"),mySounds.bg_25=createjs.Sound.createInstance("bg_25"),mySounds.bg_26=createjs.Sound.createInstance("bg_26"),mySounds.bg_27=createjs.Sound.createInstance("bg_27"),mySounds.bg_28=createjs.Sound.createInstance("bg_28"),mySounds.bg_28=createjs.Sound.createInstance("bg_28"),mySounds.bg_29=createjs.Sound.createInstance("bg_29"),mySounds.bg_30=createjs.Sound.createInstance("bg_30"),mySounds.bg_31=createjs.Sound.createInstance("bg_31"),mySounds.bg_32=createjs.Sound.createInstance("bg_32"),mySounds.bg_33=createjs.Sound.createInstance("bg_33"),mySounds.bg_34=createjs.Sound.createInstance("bg_34"),mySounds.bg_35=createjs.Sound.createInstance("bg_35"),mySounds.bg_36=createjs.Sound.createInstance("bg_36"),musicTrack1="bg_16",musicTrack2="bg_17",musicTrack3="bg_23",musicTrack4="bg_25",count=1,mySounds)mySounds.hasOwnProperty(e)&&(++count,volume_id=e+"_gain")}
"""
# # Extract file names
ogg_file_names = extract_ogg_file_names(js_code)

# # Download files
download_files(base_url, ogg_file_names, ogg_directory)

convert_to_mp3(ogg_directory,mp3_directory)