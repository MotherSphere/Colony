class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Colony")
        self.root.geometry("1280x800")
        self.root.minsize(800, 600)

        #Ici faut rajouter la première barre + Widgets

    def run(self):
        self.root.mainloop()