import tkinter as tk
from app import App

def main():
    root = tk.Tk()
    root.title("Colony")
    root.geometry("1280x800")
    root.minsize(800, 400)
    app = App(root)
    root.mainloop()

#Ici faut rajouter la première barre + Widgets (boutons, labels, etc.)

if __name__ == "__main__":
    main()