from flask import Flask
import socket

app = Flask(__name__)

C_PLUS_PLUS_IP = "127.0.0.1"
CONTROL_PORT = 6000


def send_command(command):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((C_PLUS_PLUS_IP, CONTROL_PORT))
        s.send(command.encode())
        s.close()
        return True

    except Exception as e:
        print("Connection error:", e)
        return False


@app.route("/")
def index():
    return """
    <h1>AV DSP Control</h1>

    <h2>Sharpen</h2>

    <input
        type="range"
        min="0"
        max="2"
        value="0"
        oninput="setSharpen(this.value)"
    >

    <span id="sharpenValue">0</span>

    <h2>Blur</h2>

    <input
        type="range"
        min="0"
        max="3"
        value="0"
        oninput="setBlur(this.value)"
    >

    <span id="blurValue">0</span>


    <h2>Brightness</h2>

    <input 
        type="range"
        min="-100"
        max="100"
        value="0"
        oninput="setBrightness(this.value)"
    >

    <span id="brightnessValue">0</span>


    <h2>Contrast</h2>

    <input
        type="range"
        min="0.5"
        max="3"
        step="0.1"
        value="1"
        oninput="setContrast(this.value)"
    >

    <span id="contrastValue">1</span>

    <br><br>

    <button onclick="location.href='/edge'">
        Edge
    </button>

    <button onclick="location.href='/emboss'">
        Emboss
    </button>

    <button onclick="location.href='/off'">
        Filters OFF
    </button>

    <br><br>
    <button onclick="location.href='/camera'">
    Camera
    </button>

    <button onclick="location.href='/byod'">
    BYOD
    </button>


    <script>

    function setBrightness(value)
    {
        document.getElementById("brightnessValue").innerHTML = value;

        fetch("/brightness/" + value);
    }


    function setContrast(value)
    {
        document.getElementById("contrastValue").innerHTML = value;

        fetch("/contrast/" + value);
    }

    function setSharpen(value)
    {
        document.getElementById("sharpenValue").innerHTML = value;

        fetch("/sharpen/" + value);
    }

    function setBlur(value)
    {
        document.getElementById("blurValue").innerHTML = value;

        fetch("/blur/" + value);
    }

    </script>
    """


@app.route("/sharpen/<value>")
def sharpen(value):
    send_command("sharpen_" + value)
    return "OK"

@app.route("/blur/<value>")
def blur(value):
    send_command("blur_" + value)
    return "OK"


@app.route("/edge")
def edge():
    send_command("edge_on")
    return "Edge enabled"


@app.route("/emboss")
def emboss():
    send_command("emboss_on")
    return "Emboss enabled"


@app.route("/off")
def off():
    send_command("off")
    return "Filters disabled"


@app.route("/brightness/<value>")
def brightness(value):
    send_command("brightness_" + value)
    return "Brightness updated"


@app.route("/contrast/<value>")
def contrast(value):
    send_command("contrast_" + value)
    return "Contrast updated"

@app.route("/camera")
def camera():
    send_command("source_camera")
    return "Camera selected"


@app.route("/byod")
def byod():
    send_command("source_byod")
    return "BYOD selected"



if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5006)