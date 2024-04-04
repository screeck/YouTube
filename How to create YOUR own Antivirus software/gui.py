from tkinter import *
from tkinter import filedialog
import subprocess
import ttkbootstrap as tb

root = tb.Window(themename="superhero")

root.title("żbikAV")
root.geometry("1440x900")


file_path = ""
upload_type = "file"  # Default upload type is file

def file_dialog():
    global file_path
    if upload_type == "file":
        file_path = filedialog.askopenfilename()
    else:
        file_path = filedialog.askdirectory()
    if file_path:
        file_label.config(text="Chosen path: " + file_path)
    else:
        file_label.config(text="No path selected")

def execute_engine(file_path):
    if file_path:
        # Execute engine.exe and capture output
        file_label.config(text=file_path)
        result = subprocess.run(["C:/Users/mjank/Documents/MalDev/Antivirus/engine/x64/Debug/engine.exe", file_path], stdout=subprocess.PIPE)
        output_text.delete(1.0, END)  # Clear previous output
        output_text.insert(END, result.stdout.decode())  # Insert captured output
        output_text.see(END)
    else:
        file_label.config(text="No path selected")

def toggle_upload_type():
    global upload_type
    if upload_type == "file":
        upload_type = "directory"
        toggle_button.config(text="Switch to File Upload")
    else:
        upload_type = "file"
        toggle_button.config(text="Switch to Directory Upload")

my_label = tb.Label(text="Best AV in the world", font=("Helvetica", 40), bootstyle="default")
my_label.pack(pady=10)

# Add a toggle button to switch between file and directory upload
toggle_button = tb.Button(text="Switch to Directory Upload", bootstyle="secondary", command=toggle_upload_type)
toggle_button.pack(pady=10)

file_label = tb.Label(text="", font=("Helvetica", 12), bootstyle="default")
file_label.pack(pady=10)

# Load your initial image
image = PhotoImage(file="images/upload_image.png")

# Create a label with the image
image_button = tb.Label(image=image)
image_button.pack(pady=10)

# Bind the click event of the image to the file_dialog function
image_button.bind("<Button-1>", lambda event: file_dialog())

# Use lambda function to pass file_path to execute_engine when my_button is clicked
my_button = tb.Button(text="Upload", bootstyle="primary, outline", command=lambda: execute_engine(file_path))
my_button.config(padding="40 15")
my_button.pack(pady=20)

my_label = tb.Label(text="Scan results", font=("Helvetica", 30), bootstyle="default")
my_label.pack(pady=20)

# Create a text widget to display the output
output_text = Text(root, width=200, height=30, wrap='word', font=("Helvetica", 14))
output_text.pack(pady=10)


root.mainloop()
