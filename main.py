import tkinter as tk
from tkinter import ttk, filedialog, font

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

    # State: path of the currently opened/saved file
    current_file_path = {"path": None}

    # Save to current file (if any), otherwise ask Save As
    def save():
        path = current_file_path["path"]
        if not path:
            return save_as()
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text_widget.get("1.0", "end-1c"))
        except Exception as e:
            tk.messagebox = getattr(tk, "messagebox", None)
            # fallback: show simple dialog if messagebox available
            try:
                from tkinter import messagebox
                messagebox.showerror("Erreur", f"Impossible d'enregistrer : {e}")
            except Exception:
                pass

    # Save As: always prompt for filename
    def save_as():
        path = filedialog.asksaveasfilename(defaultextension=".txt",
                                            filetypes=[("Text files", "*.txt"), ("All files", "*.*")])
        if not path:
            return
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(text_widget.get("1.0", "end-1c"))
            current_file_path["path"] = path
            root.title(f"Colony - {path}")
        except Exception as e:
            try:
                from tkinter import messagebox
                messagebox.showerror("Erreur", f"Impossible d'enregistrer : {e}")
            except Exception:
                pass

    # Open file and remember path
    def open_file():
        path = filedialog.askopenfilename(filetypes=[("Text files", "*.txt"), ("All files", "*.*")])
        if path:
            try:
                with open(path, "r", encoding="utf-8") as f:
                    content = f.read()
                text_widget.delete("1.0", "end")
                text_widget.insert("1.0", content)
                current_file_path["path"] = path
                root.title(f"Colony - {path}")
            except Exception as e:
                try:
                    from tkinter import messagebox
                    messagebox.showerror("Erreur", f"Impossible d'ouvrir le fichier : {e}")
                except Exception:
                    pass

    # Menu pour ouvrir/enregistrer
    menubar = tk.Menu(root)
    file_menu = tk.Menu(menubar, tearoff=0)

    file_menu.add_command(label="Ouvrir...", command=open_file, accelerator="Ctrl+O")
    file_menu.add_command(label="Enregistrer", command=save, accelerator="Ctrl+S")
    file_menu.add_command(label="Enregistrer sous...", command=save_as, accelerator="Ctrl+Shift+S")
    file_menu.add_separator()
    # Options placeholder (no implementation in this file)
    file_menu.add_command(label="Options...", command=lambda: None)
    file_menu.add_command(label="Quitter", command=root.quit)
    menubar.add_cascade(label="Fichier", menu=file_menu)
    root.config(menu=menubar)

    # Shortcuts
    root.bind_all("<Control-s>", lambda e: save())
    # Ctrl+Shift+S can be represented as Control-Shift-S (works on many platforms)
    root.bind_all("<Control-Shift-S>", lambda e: save_as())
    root.bind_all("<Control-o>", lambda e: open_file())

    root.mainloop()

if __name__ == "__main__":
    main()