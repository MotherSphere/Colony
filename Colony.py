import tkinter as tk
from tkinter import ttk, filedialog, font
from app import App

def main():
    root = tk.Tk()
    root.title("Colony")
    root.geometry("1280x800")
    root.minsize(800, 400)

    txt_font = font.Font(family="Segoe UI", size=14)
    text_widget = tk.Text(root, wrap="word", font=txt_font, undo=True, borderwidth=0)
    vsb = ttk.Scrollbar(root, orient="vertical", command=text_widget.yview)
    text_widget.configure(yscrollcommand=vsb.set)
    vsb.pack(side="right", fill="y")
    text_widget.pack(fill="both", expand=True, padx=10, pady=10)

    # Menu pour ouvrir/enregistrer (Ajouter enregistrer sous)
    menubar = tk.Menu(root)
    file_menu = tk.Menu(menubar, tearoff=0)

    def save_file():
        path = filedialog.asksaveasfilename(defaultextension=".txt", filetypes=[("Text files","*.txt"),("All files","*.*")])
        if path:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text_widget.get("1.0", "end-1c"))

    def open_file():
        path = filedialog.askopenfilename(filetypes=[("Text files","*.txt"),("All files","*.*")])
        if path:
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
            text_widget.delete("1.0", "end")
            text_widget.insert("1.0", content)

    file_menu.add_command(label="Ouvrir...", command=open_file)
    file_menu.add_command(label="Enregistrer...", command=save_file)
    file_menu.add_separator()
    file_menu.add_command(label="Quitter", command=root.quit)
    menubar.add_cascade(label="Fichier", menu=file_menu)
    root.config(menu=menubar)

    root.mainloop()

if __name__ == "__main__":
    main()
