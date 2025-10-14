import tkinter as tk
from tkinter import ttk, filedialog, messagebox, font, simpledialog
import os, json, re, pathlib, webbrowser, uuid, hashlib, base64, datetime, platform, subprocess, shutil, math, itertools, threading, queue, ast, difflib, csv, xml.etree.ElementTree as ET
from tkinter.scrolledtext import ScrolledText
from tkinter import colorchooser
# ---------------------------------------------------------------------------
#  Si une lib manque → fallback silencieux (sauf Pillow pour images)
# ---------------------------------------------------------------------------
MISSING = []
try:
    import pygments, pygments.lexers, pygments.styles   # pip install pygments
    PYGMENTS = True
except ImportError:
    PYGMENTS = False
    MISSING.append("pygments")

# Optional third-party libraries (non-fatal)
try:
    from PIL import Image, ImageTk
except Exception:
    Image = ImageTk = None
    MISSING.append("Pillow")

try:
    import pyperclip
except Exception:
    pyperclip = None
    MISSING.append("pyperclip")

try:
    import qrcode
except Exception:
    qrcode = None
    MISSING.append("qrcode")

try:
    import pyfiglet
except Exception:
    pyfiglet = None

# ---------------------------------------------------------------------------
#  Langues
# ---------------------------------------------------------------------------
LANG = {
"fr": {"file":"Fichier","options":"Options…","quit":"Quitter","coloration":"Coloration","edit":"Édition","view":"Vue","help":"Aide","about":"À propos","palette":"Palette de commandes","explorer":"Explorateur","terminal":"Terminal","save":"Enregistrer","save_as":"Enregistrer sous","new":"Nouveau","open":"Ouvrir…","recent":"Récents","undo":"Annuler","redo":"Refaire","cut":"Couper","copy":"Copier","paste":"Coller","find":"Rechercher","replace":"Remplacer","goto":"Aller à la ligne","select_all":"Tout sélectionner","zoom_in":"Zoom +","zoom_out":"Zoom –","word_wrap":"Retour à la ligne","line_numbers":"Numéros de ligne","syntax_on":"Coloration syntaxique","dark_mode":"Mode sombre","light_mode":"Mode clair","minimap":"Minimap","breadcrumb":"Fil d’Ariane","sticky_scroll":"Défilement collant","inlay_hints":"Indices inline","bracket_pairs":"Couleurs des paires","format_on_save":"Formater à la sauvegarde","trim_trailing":"Supprimer espaces finaux","insert_final_newline":"Nouvelle ligne finale","auto_save":"Sauvegarde auto","auto_save_delay":"Délai (ms)","tab_size":"Taille tabulation","indent_with_spaces":"Indenter avec espaces","show_whitespace":"Afficher espaces","rulers":"Règles","font_size":"Taille police","font_family":"Police","editor":"Éditeur","appearance":"Apparence","behavior":"Comportement","languages":"Langages","shortcuts":"Raccourcis","extensions":"Extensions","git":"Git","terminal_shell":"Shell terminal","terminal_font":"Police terminal","restore_defaults":"Restaurer défauts","close":"Fermer","search_settings":"Rechercher paramètres…","filter":"Filtrer","reset":"Réinitialiser","apply":"Appliquer","import_settings":"Importer…","export_settings":"Exporter…","ascii_art":"Art ASCII","qr_code":"QR Code","base64_encode":"Base64 encoder","base64_decode":"Base64 décoder","hash_md5":"Hash MD5","hash_sha256":"Hash SHA256","timestamp":"Timestamp Unix","uuid_gen":"UUID","color_picker":"Sélecteur couleur","emoji_panel":"Panneau emoji","latex_math":"Formule LaTeX","plantuml":"PlantUML","mermaid":"Mermaid","graphviz":"Graphviz","csv_viewer":"Visionneur CSV","json_tree":"Arbre JSON","xml_tree":"Arbre XML","diff_viewer":"Visionneur Diff","git_blame":"Git Blame","git_log":"Git Log","git_graph":"Git Graph","system_info":"Info système","python":"Python","javascript":"JavaScript","typescript":"TypeScript","html":"HTML","css":"CSS","json":"JSON","markdown":"Markdown","cpp":"C++","java":"Java","go":"Go","rust":"Rust","php":"PHP","sql":"SQL","xml":"XML","yaml":"YAML","toml":"TOML","dockerfile":"Dockerfile","powershell":"PowerShell","bash":"Bash","lua":"Lua","ruby":"Ruby","r":"R","julia":"Julia","kotlin":"Kotlin","swift":"Swift","dart":"Dart","scala":"Scala","perl":"Perl","haskell":"Haskell","clojure":"Clojure","elixir":"Elixir","erlang":"Erlang","fsharp":"F#","csharp":"C#","vbnet":"VB.NET","typescriptreact":"TypeScript React","javascriptreact":"JavaScript React","vue":"Vue","svelte":"Svelte","astro":"Astro","solid":"Solid","jsonc":"JSON avec commentaires","json5":"JSON5","sass":"Sass","scss":"SCSS","less":"Less","stylus":"Stylus","postcss":"PostCSS","tailwind":"Tailwind CSS","webpack":"Webpack","vite":"Vite","nodejs":"Node.js","react":"React","angular":"Angular","nextjs":"Next.js","nuxtjs":"Nuxt.js","gatsby":"Gatsby","sveltekit":"SvelteKit","remix":"Remix","quasar":"Quasar","vuetify":"Vuetify","bootstrap":"Bootstrap","bulma":"Bulma","foundation":"Foundation","semantic_ui":"Semantic UI","uikit":"UIKit","materialize":"Materialize","antd":"Ant Design","element_ui":"Element UI","chakra_ui":"Chakra UI","mantine":"Mantine","primefaces":"PrimeFaces","ionic":"Ionic","flutter":"Flutter","android":"Android","ios":"iOS","xamarin":"Xamarin","react_native":"React Native","native_script":"NativeScript","capacitor":"Capacitor","cordova":"Cordova","phonegap":"PhoneGap","electron":"Electron","tauri":"Tauri","nwjs":"NW.js","neutralino":"Neutralino","wails":"Wails","gin":"Gin","echo":"Echo","fiber":"Fiber","beego":"Beego","revel":"Revel","martini":"Martini","chi":"Chi","mux":"Mux","httprouter":"HttpRouter","gorilla":"Gorilla","fasthttp":"FastHTTP","rocket":"Rocket","actix":"Actix","axum":"Axum","warp":"Warp","tide":"Tide","hyper":"Hyper","reqwest":"Reqwest","surf":"Surf","curl":"cURL","wget":"Wget","httpie":"HTTPie","postman":"Postman","insomnia":"Insomnia","paw":"Paw","charles":"Charles","fiddler":"Fiddler","burp":"Burp Suite","wireshark":"Wireshark","tcpdump":"Tcpdump","nmap":"Nmap","metasploit":"Metasploit","nexus":"Nexus","jira":"Jira","confluence":"Confluence","bitbucket":"Bitbucket","gitlab":"GitLab","github":"GitHub","sourceforge":"SourceForge","google_code":"Google Code","codeplex":"CodePlex","launchpad":"Launchpad"," Savannah":"GNU Savannah","openhub":"Open Hub","stack Overflow":"Stack Overflow","reddit":"Reddit","twitter":"Twitter","linkedin":"LinkedIn","facebook":"Facebook","instagram":"Instagram","youtube":"YouTube","twitch":"Twitch","discord":"Discord","slack":"Slack","teams":"Microsoft Teams","zoom":"Zoom","skype":"Skype","whatsapp":"WhatsApp","telegram":"Telegram","signal":"Signal","element":"Element","matrix":"Matrix","mastodon":"Mastodon","diaspora":"Diaspora","friendica":"Friendica","hubzilla":"Hubzilla","gnu_social":"GNU Social","pleroma":"Pleroma","misskey":"Misskey","peertube":"PeerTube","pixelfed":"PixelFed","writefreely":"WriteFreely","plume":"Plume","ghost":"Ghost","hexo":"Hexo","hugo":"Hugo","jekyll":"Jekyll","gatsby":"Gatsby","next":"Next.js","nuxt":"Nuxt.js"," gridsome":"Gridsome","vuepress":"VuePress","docusaurus":"Docusaurus","gitbook":"GitBook","mkdocs":"MkDocs","sphinx":"Sphinx","pdoc":"pDoc","jsdoc":"JSDoc","esdoc":"ESDoc","typedoc":"TypeDoc","compodoc":"Compodoc","storybook":"Storybook","styleguidist":"Styleguidist","pattern_lab":"Pattern Lab","fractal":"Fractal","knapsack":"Knapsack","bit":"Bit","lerna":"Lerna","nx":"Nx","rush":"Rush","turborepo":"Turborepo","ultra":"Ultra","npm":"npm","yarn":"Yarn","pnpm":"pnpm","bun":"Bun","volta":"Volta","fnm":"fnm","nvm":"nvm","asdf":"asdf","g":"g","n":"n","fish_nvm":"Fish nvm","zsh_nvm":"Zsh nvm","bash_nvm":"Bash nvm","profile_nvm":"Profile nvm","nvm_dir":"NVM_DIR","node_path":"NODE_PATH","npm_config_prefix":"npm_config_prefix","npm_config_cache":"npm_config_cache","npm_config_tmp":"npm_config_tmp","npm_config_init_module":"npm_config_init_module","npm_config_userconfig":"npm_config_userconfig","npm_config_globalconfig":"npm_config_globalconfig","npm_config_global_style":"npm_config_global_style","npm_config_package_lock":"npm_config_package_lock","npm_config_shrinkwrap":"npm_config_shrinkwrap","npm_config_unicode":"npm_config_unicode","npm_config_loglevel":"npm_config_loglevel","npm_config_progress":"npm_config_progress","npm_config_color":"npm_config_color","npm_config_strict_ssl":"npm_config_strict_ssl","npm_config_ca":"npm_config_ca","npm_config_cafile":"npm_config_cafile","npm_config_cert":"npm_config_cert","npm_config_key":"npm_config_key","npm_config_local_address":"npm_config_local_address","npm_config_config":"npm_config_config","npm_config_depth":"npm_config_depth","npm_config_description":"npm_config_description","npm_config_dev":"npm_config_dev","npm_config_dry_run":"npm_config_dry_run","npm_config_force":"npm_config_force","npm_config_git":"npm_config_git","npm_config_global":"npm_config_global","npm_config_group":"npm_config_group","npm_config_ignore_scripts":"npm_config_ignore_scripts","npm_config_init_author_email":"npm_config_init_author_email","npm_config_init_author_name":"npm_config_init_author_name","npm_config_init_author_url":"npm_config_init_author_url","npm_config_init_license":"npm_config_init_license","npm_config_init_version":"npm_config_init_version","npm_config_json":"npm_config_json","npm_config_link":"npm_config_link","npm_config_loglevel":"npm_config_loglevel","npm_config_long":"npm_config_long","npm_config_message":"npm_config_message","npm_config_no_optional":"npm_config_no_optional","npm_config_no_shrinkwrap":"npm_config_no_shrinkwrap","npm_config_no_optional":"npm_config_no_optional","npm_config_node_options":"npm_config_node_options","npm_config_node_version":"npm_config_node_version","npm_config_noproxy":"npm_config_noproxy","npm_config_offline":"npm_config_offline","npm_config_only":"npm_config_only","npm_config_optional":"npm_config_optional","npm_config_otp":"npm_config_otp","npm_config_package_lock_only":"npm_config_package_lock_only","npm_config_parseable":"npm_config_parseable","npm_config_prefer_offline":"npm_config_prefer_offline","npm_config_prefer_online":"npm_config_prefer_online","npm_config_prefix":"npm_config_prefix","npm_config_production":"npm_config_production","npm_config_progress":"npm_config_progress","npm_config_proxy":"npm_config_proxy","npm_config_read_only":"npm_config_read_only","npm_config_rebuild_bundle":"npm_config_rebuild_bundle","npm_config_registry":"npm_config_registry","npm_configrollback":"npm_config_rollback","npm_config_save":"npm_config_save","npm_config_save_bundle":"npm_config_save_bundle","npm_config_save_dev":"npm_config_save_dev","npm_config_save_exact":"npm_config_save_exact","npm_config_save_optional":"npm_config_save_optional","npm_config_save_prefix":"npm_config_save_prefix","npm_config_save_prod":"npm_config_save_prod","npm_config_scope":"npm_config_scope","npm_config_scripts_prepend_node_path":"npm_config_scripts_prepend_node_path","npm_config_script_shell":"npm_config_script_shell","npm_config_searchexclude":"npm_config_searchexclude","npm_config_searchlimit":"npm_config_searchlimit","npm_config_searchopts":"npm_config_searchopts","npm_config_searchstaleness":"npm_config_searchstaleness","npm_config_send_metrics":"npm_config_send_metrics","npm_config_shell":"npm_config_shell","npm_config_shrinkwrap":"npm_config_shrinkwrap","npm_config_sign_git_commit":"npm_config_sign_git_commit","npm_config_sign_git_tag":"npm_config_sign_git_tag","npm_config_sso_poll_frequency":"npm_config_sso_poll_frequency","npm_config_sso_type":"npm_config_sso_type","npm_config_strict_ssl":"npm_config_strict_ssl","npm_config_tag":"npm_config_tag","npm_config_tag_version_prefix":"npm_config_tag_version_prefix","npm_config_timing":"npm_config_timing","npm_config_tmp":"npm_config_tmp","npm_config_umask":"npm_config_umask","npm_config_unicode":"npm_config_unicode","npm_config unsafe_perm":"npm_config_unsafe_perm","npm_config_update_notifier":"npm_config_update_notifier","npm_config_usage":"npm_config_usage","npm_config_user":"npm_config_user","npm_config_userconfig":"npm_config_userconfig","npm_config_username":"npm_config_username","npm_config_user_agent":"npm_config_user_agent","npm_config_version":"npm_config_version","npm_config_versions":"npm_config_versions","npm_config_viewer":"npm_config_viewer","npm_config_yes":"npm_config_yes","npm_config_yes":"npm_config_yes"}
}

# ---------------------------------------------------------------------------
#  180 langages pour le menu « Coloration »
# ---------------------------------------------------------------------------
LANGS = [
("Python", "python", ".py"),
("JavaScript", "javascript", ".js"),
("TypeScript", "typescript", ".ts"),
("JSX", "javascriptreact", ".jsx"),
("TSX", "typescriptreact", ".tsx"),
("Vue", "vue", ".vue"),
("Svelte", "svelte", ".svelte"),
("Astro", "astro", ".astro"),
("HTML", "html", ".html"),
("CSS", "css", ".css"),
("SCSS", "scss", ".scss"),
("Sass", "sass", ".sass"),
("Less", "less", ".less"),
("Stylus", "stylus", ".styl"),
("PostCSS", "postcss", ".pcss"),
("Tailwind", "tailwind", ".tailwind"),
("JSON", "json", ".json"),
("JSONC", "jsonc", ".jsonc"),
("JSON5", "json5", ".json5"),
("YAML", "yaml", ".yaml"),
("YML", "yaml", ".yml"),
("TOML", "toml", ".toml"),
("XML", "xml", ".xml"),
("SVG", "xml", ".svg"),
("MathML", "xml", ".mml"),
("Markdown", "markdown", ".md"),
("LaTeX", "latex", ".tex"),
("BibTeX", "bibtex", ".bib"),
("Dockerfile", "dockerfile", "Dockerfile"),
("Docker Compose", "dockercompose", "docker-compose.yml"),
("C", "c", ".c"),
("C++", "cpp", ".cpp"),
("C#", "csharp", ".cs"),
("Java", "java", ".java"),
("Kotlin", "kotlin", ".kt"),
("Scala", "scala", ".scala"),
("Groovy", "groovy", ".groovy"),
("Go", "go", ".go"),
("Rust", "rust", ".rs"),
("Zig", "zig", ".zig"),
("V", "v", ".v"),
("Nim", "nim", ".nim"),
("Crystal", "crystal", ".cr"),
("Ruby", "ruby", ".rb"),
("PHP", "php", ".php"),
("Swift", "swift", ".swift"),
("Dart", "dart", ".dart"),
("Objective-C", "objective-c", ".m"),
("Perl", "perl", ".pl"),
("R", "r", ".r"),
("Julia", "julia", ".jl"),
("Lua", "lua", ".lua"),
("PowerShell", "powershell", ".ps1"),
("Bash", "shellscript", ".sh"),
("Batch", "bat", ".bat"),
("SQL", "sql", ".sql"),
("GraphQL", "graphql", ".graphql"),
("Prisma", "prisma", ".prisma"),
("Cypher", "cypher", ".cyp"),
("Terraform", "terraform", ".tf"),
("Ansible", "ansible", ".yml"),
("Nginx", "nginx", ".conf"),
("Apache", "apache", ".conf"),
("SystemD", "systemd", ".service"),
("Makefile", "makefile", "Makefile"),
("CMake", "cmake", "CMakeLists.txt"),
("Gradle", "gradle", "build.gradle"),
("Maven", "xml", "pom.xml"),
("SBT", "sbt", "build.sbt"),
("Leiningen", "clojure", "project.clj"),
("Mix", "elixir", "mix.exs"),
("Cargo", "toml", "Cargo.toml"),
("Composer", "json", "composer.json"),
("Gemfile", "ruby", "Gemfile"),
("Package.json", "json", "package.json"),
("Tsconfig", "jsonc", "tsconfig.json"),
("Vite", "javascript", "vite.config.js"),
("Webpack", "javascript", "webpack.config.js"),
("Rollup", "javascript", "rollup.config.js"),
("Parcel", "json", ".parcelrc"),
("ESLint", "json", ".eslintrc.json"),
("Prettier", "json", ".prettierrc.json"),
("Jest", "javascript", "jest.config.js"),
("Mocha", "javascript", "mocha.opts"),
("Cypress", "javascript", "cypress.config.js"),
("Playwright", "javascript", "playwright.config.js"),
("Storybook", "javascript", ".storybook/main.js"),
("Git", "git", ".gitignore"),
("Git Attributes", "git", ".gitattributes"),
("Docker Ignore", "dockerfile", ".dockerignore"),
("NPM Ignore", "git", ".npmignore"),
("YAML GitHub", "yaml", ".github/workflows/*.yml"),
("Neovim", "vim", ".nvim"),
("Vim", "vim", ".vim"),
("Emacs", "elisp", ".el"),
("Org Mode", "org", ".org"),
("CSV", "csv", ".csv"),
("TSV", "csv", ".tsv"),
("INI", "ini", ".ini"),
("CFG", "ini", ".cfg"),
("CONF", "ini", ".conf"),
("Properties", "ini", ".properties"),
("Env", "ini", ".env"),
("Log", "log", ".log"),
("Diff", "diff", ".diff"),
("Patch", "diff", ".patch"),
("ASCII", "text", ".txt"),
("Text", "text", ".text"),
("RTF", "rtf", ".rtf"),
("LaTeX Beamer", "latex", ".beamer"),
("LaTeX Article", "latex", ".article"),
("LaTeX Report", "latex", ".report"),
("LaTeX Book", "latex", ".book"),
("BibLaTeX", "bibtex", ".biblatex"),
("AMS LaTeX", "latex", ".ams"),
("TikZ", "latex", ".tikz"),
("PGF", "latex", ".pgf"),
("Metapost", "metapost", ".mp"),
("Asymptote", "asymptote", ".asy"),
("Gnuplot", "gnuplot", ".gp"),
("Matlab", "matlab", ".m"),
("Octave", "octave", ".m"),
("Scilab", "scilab", ".sce"),
("R Markdown", "r", ".Rmd"),
("Quarto", "markdown", ".qmd"),
("Jupyter", "json", ".ipynb"),
("Notebook", "json", ".notebook"),
("Pascal", "pascal", ".pas"),
("Delphi", "pascal", ".dpr"),
("Ada", "ada", ".adb"),
("Fortran", "fortran", ".f90"),
("COBOL", "cobol", ".cbl"),
("Lisp", "lisp", ".lisp"),
("Scheme", "scheme", ".scm"),
("Racket", "racket", ".rkt"),
("Prolog", "prolog", ".pl"),
("Erlang", "erlang", ".erl"),
("Elixir", "elixir", ".ex"),
("Gleam", "gleam", ".gleam"),
("Elm", "elm", ".elm"),
("PureScript", "purescript", ".purs"),
("Reason", "reason", ".re"),
("ReScript", "rescript", ".res"),
("CoffeeScript", "coffeescript", ".coffee"),
("LiveScript", "livescript", ".ls"),
("ClojureScript", "clojure", ".cljs"),
("TypeScript Declaration", "typescript", ".d.ts"),
("Assembly", "asm", ".asm"),
("LLVM IR", "llvm", ".ll"),
("WebAssembly", "wasm", ".wat"),
("GLSL", "glsl", ".glsl"),
("HLSL", "hlsl", ".hlsl"),
("OpenCL", "opencl", ".cl"),
("CUDA", "cuda", ".cu"),
("Metal", "metal", ".metal"),
("Vega", "json", ".vg.json"),
("Vega-Lite", "json", ".vl.json"),
("D3", "javascript", ".d3.js"),
("Three.js", "javascript", ".three.js"),
("Babylon.js", "javascript", ".babylon.js"),
("Phaser", "javascript", ".phaser.js"),
("PixiJS", "javascript", ".pixi.js"),
("PlayCanvas", "javascript", ".playcanvas.js"),
("Cocos2d", "javascript", ".cocos2d.js"),
("Unity", "csharp", ".unity"),
("Unreal", "cpp", ".uproject"),
("Godot", "gdscript", ".gd"),
("GDScript", "gdscript", ".gd"),
("GDNative", "cpp", ".gdnlib"),
("GDNativeLibrary", "cpp", ".gdnlib"),
("GDNativeScript", "cpp", ".gdns"),
("GDNativeLibraryResource", "cpp", ".gdnlib"),
("GDNativeScriptResource", "cpp", ".gdns")
]

# ---------------------------------------------------------------------------
#  Thèmes Material Design 3
# ---------------------------------------------------------------------------
THEMES = {
"light": {"bg":"#FFFFFF","fg":"#212121","primary":"#1976D2","secondary":"#FF4081","surface":"#F5F5F5","border":"#E0E0E0","text_bg":"#FFFFFF","text_fg":"#212121","select_bg":"#E3F2FD","select_fg":"#1976D2","status_bg":"#1976D2","status_fg":"#FFFFFF","menu_bg":"#FAFAFA","menu_fg":"#212121","menu_select":"#E3F2FD","button_bg":"#1976D2","button_fg":"#FFFFFF","entry_bg":"#FFFFFF","entry_fg":"#212121","margin_bg":"#FAFAFA","margin_fg":"#757575","find_bg":"#FFF3E0"},
"dark": {"bg":"#121212","fg":"#E0E0E0","primary":"#90CAF9","secondary":"#F48FB1","surface":"#1E1E1E","border":"#333333","text_bg":"#1E1E1E","text_fg":"#E0E0E0","select_bg":"#264F78","select_fg":"#FFFFFF","status_bg":"#1E1E1E","status_fg":"#90CAF9","menu_bg":"#1E1E1E","menu_fg":"#E0E0E0","menu_select":"#264F78","button_bg":"#90CAF9","button_fg":"#121212","entry_bg":"#1E1E1E","entry_fg":"#E0E0E0","margin_bg":"#1E1E1E","margin_fg":"#757575","find_bg":"#2D2D2D"}
}

# ---------------------------------------------------------------------------
#  Config par défaut (sauvegardée dans colony.json)
# ---------------------------------------------------------------------------
DEFAULT_CFG = {
"language":"fr",
"theme":"light",
"font_size":14,
"font_family":"Segoe UI",
"word_wrap":True,
"show_line_numbers":True,
"show_whitespace":False,
"indent_with_spaces":True,
"tab_size":4,
"trim_trailing":True,
"insert_final_newline":True,
"format_on_save":False,
"auto_save":False,
"auto_save_delay":300000,
"rulers":[80,120],
"minimap":False,
"sticky_scroll":False,
"breadcrumb":False,
"inlay_hints":False,
"bracket_pairs":True,
"current_lexer":"auto",
"terminal_shell":"cmd" if os.name=="nt" else "bash",
"terminal_font":"Consolas 11",
"recent_files":[],
"shortcuts":{}
}

# ---------------------------------------------------------------------------
#  Classes utilitaires
# ---------------------------------------------------------------------------
class MaterialButton(ttk.Button):
    def __init__(self, parent, text="", command=None, style="Material.TButton", **kw):
        super().__init__(parent, text=text, command=command, style=style, **kw)

class MaterialText(tk.Text):
    def __init__(self, parent, **kw):
        super().__init__(parent, **kw)
        self.setup_material()
    def setup_material(self):
        self.configure(wrap="word", undo=True, maxundo=-1, autoseparators=True, insertwidth=2, selectborderwidth=0, inactiveselectbackground="#E3F2FD", padx=10, pady=10)

# ---------------------------------------------------------------------------
#  Application principale
# ---------------------------------------------------------------------------
class Colony(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Colony - Mother Sphere Text Editor")
        self.geometry("1600x900")
        self.minsize(1200, 700)
        self.current_file = None
        self.modified = False
        self.cfg = self.load_cfg()
        self.theme = self.cfg["theme"]
        self.language = self.cfg["language"]
        self.recent_files = self.cfg["recent_files"]
        self.current_lexer = self.cfg["current_lexer"]
        self.setup_style()
        self.build_ui()
        self.bind_all_shortcuts()
        self.protocol("WM_DELETE_WINDOW", self.on_exit)
        self.focus_force()

    # -----------------------------------------------------------------------
    #  Configuration
    # -----------------------------------------------------------------------
    def load_cfg(self):
        if os.path.exists("colony.json"):
            try:
                with open("colony.json", "r", encoding="utf-8") as f:
                    return {**DEFAULT_CFG, **json.load(f)}
            except:
                pass
        return DEFAULT_CFG.copy()

    def save_cfg(self):
        try:
            with open("colony.json", "w", encoding="utf-8") as f:
                json.dump(self.cfg, f, indent=2, ensure_ascii=False)
        except:
            pass

    # -----------------------------------------------------------------------
    #  Style Material Design 3
    # -----------------------------------------------------------------------
    def setup_style(self):
        self.ttk_style = ttk.Style()
        self.ttk_style.theme_use("clam")
        t = THEMES[self.theme]
        self.configure(bg=t["bg"])
        self.ttk_style.configure("Material.TButton", background=t["button_bg"], foreground=t["button_fg"], borderwidth=0, focuscolor="none", relief="flat", padding=(8, 4))
        self.ttk_style.map("Material.TButton", background=[("active", t["primary"]), ("pressed", t["secondary"])])
        self.ttk_style.configure("Material.TLabel", background=t["bg"], foreground=t["fg"])
        self.ttk_style.configure("Material.TFrame", background=t["bg"])
        self.ttk_style.configure("Material.TCheckbutton", background=t["bg"], foreground=t["fg"])
        self.ttk_style.configure("Material.TRadiobutton", background=t["bg"], foreground=t["fg"])
        self.ttk_style.configure("Material.TEntry", fieldbackground=t["entry_bg"], foreground=t["entry_fg"], borderwidth=1, relief="flat", insertcolor=t["entry_fg"])
        self.ttk_style.configure("Material.TCombobox", fieldbackground=t["entry_bg"], foreground=t["entry_fg"], borderwidth=1, relief="flat")
        self.ttk_style.configure("Material.TSpinbox", fieldbackground=t["entry_bg"], foreground=t["entry_fg"], borderwidth=1, relief="flat")
        self.ttk_style.configure("Treeview", background=t["text_bg"], foreground=t["text_fg"], fieldbackground=t["text_bg"], borderwidth=0)
        self.ttk_style.map("Treeview", background=[("selected", t["select_bg"])], foreground=[("selected", t["select_fg"])])

    # -----------------------------------------------------------------------
    #  Interface
    # -----------------------------------------------------------------------
    def build_ui(self):
        self.build_menu()
        self.build_toolbar()
        self.build_paned()
        self.build_status()
        self.apply_theme_to_text()

    # -----------------------------------------------------------------------
    #  Menu
    # -----------------------------------------------------------------------
    def build_menu(self):
        t = THEMES[self.theme]
        men = tk.Menu(self, bg=t["menu_bg"], fg=t["menu_fg"], activebackground=t["menu_select"], activeforeground=t["fg"], relief="flat", borderwidth=0)
        self.config(menu=men)

        # Fichier
        file = tk.Menu(men, tearoff=0, **self.menu_opts())
        men.add_cascade(label=LANG[self.language]["file"], menu=file)
        file.add_command(label=LANG[self.language]["new"], command=self.new_file, accelerator="Ctrl+N")
        file.add_command(label=LANG[self.language]["open"], command=self.open_file, accelerator="Ctrl+O")
        file.add_command(label=LANG[self.language]["save"], command=self.save_file, accelerator="Ctrl+S")
        file.add_command(label=LANG[self.language]["save_as"], command=self.save_as_file, accelerator="Ctrl+Shift+S")
        file.add_separator()
        self.recent_menu = tk.Menu(file, tearoff=0, **self.menu_opts())
        file.add_cascade(label=LANG[self.language]["recent"], menu=self.recent_menu)
        self.update_recent_menu()
        file.add_separator()
        file.add_command(label=LANG[self.language]["options"], command=self.show_options_dialog)
        file.add_separator()
        file.add_command(label=LANG[self.language]["quit"], command=self.on_exit)

        # Coloration
        syntax = tk.Menu(men, tearoff=0, **self.menu_opts())
        men.add_cascade(label=LANG[self.language]["coloration"], menu=syntax)
        self.build_syntax_menu(syntax)

        # Édition
        edit = tk.Menu(men, tearoff=0, **self.menu_opts())
        men.add_cascade(label=LANG[self.language]["edit"], menu=edit)
        edit.add_command(label=LANG[self.language]["undo"], command=self.undo, accelerator="Ctrl+Z")
        edit.add_command(label=LANG[self.language]["redo"], command=self.redo, accelerator="Ctrl+Y")
        edit.add_separator()
        edit.add_command(label=LANG[self.language]["cut"], command=self.cut, accelerator="Ctrl+X")
        edit.add_command(label=LANG[self.language]["copy"], command=self.copy, accelerator="Ctrl+C")
        edit.add_command(label=LANG[self.language]["paste"], command=self.paste, accelerator="Ctrl+V")
        edit.add_command(label=LANG[self.language]["select_all"], command=self.select_all, accelerator="Ctrl+A")
        edit.add_separator()
        edit.add_command(label=LANG[self.language]["find"], command=self.show_find, accelerator="Ctrl+F")
        edit.add_command(label=LANG[self.language]["replace"], command=self.show_replace, accelerator="Ctrl+H")
        edit.add_command(label=LANG[self.language]["goto"], command=self.goto_line, accelerator="Ctrl+G")

        # Vue
        view = tk.Menu(men, tearoff=0, **self.menu_opts())
        men.add_cascade(label=LANG[self.language]["view"], menu=view)
        view.add_command(label=LANG[self.language]["zoom_in"], command=self.zoom_in, accelerator="Ctrl++")
        view.add_command(label=LANG[self.language]["zoom_out"], command=self.zoom_out, accelerator="Ctrl+-")
        view.add_separator()
        self.wrap_var = tk.BooleanVar(value=self.cfg["word_wrap"])
        view.add_checkbutton(label=LANG[self.language]["word_wrap"], onvalue=True, offvalue=False, variable=self.wrap_var, command=self.toggle_wrap)
        self.num_var = tk.BooleanVar(value=self.cfg["show_line_numbers"])
        view.add_checkbutton(label=LANG[self.language]["line_numbers"], onvalue=True, offvalue=False, variable=self.num_var, command=self.toggle_line_numbers)
        self.syntax_var = tk.BooleanVar(value=True)
        view.add_checkbutton(label=LANG[self.language]["syntax_on"], onvalue=True, offvalue=False, variable=self.syntax_var, command=self.toggle_syntax)
        view.add_separator()
        view.add_command(label=LANG[self.language]["dark_mode"] if self.theme=="light" else LANG[self.language]["light_mode"], command=self.toggle_theme)

        # Aide
        help = tk.Menu(men, tearoff=0, **self.menu_opts())
        men.add_cascade(label=LANG[self.language]["help"], menu=help)
        help.add_command(label=LANG[self.language]["about"], command=self.show_about)

    def menu_opts(self):
        t = THEMES[self.theme]
        return {"bg":t["menu_bg"], "fg":t["menu_fg"], "activebackground":t["menu_select"], "activeforeground":t["fg"], "relief":"flat", "borderwidth":0}

    # -----------------------------------------------------------------------
    #  Menu Coloration (180 langages)
    # -----------------------------------------------------------------------
    def build_syntax_menu(self, parent_menu):
        # On groupe par familles pour éviter un menu trop long
        families = {
            "Web": ["html", "css", "scss", "sass", "less", "stylus", "postcss", "javascript", "typescript", "javascriptreact", "typescriptreact", "vue", "svelte", "astro", "json", "jsonc", "json5", "yaml", "xml", "svg"],
            "Systems": ["c", "cpp", "rust", "go", "zig", "v", "nim", "crystal", "assembly", "llvm", "webassembly"],
            "JVM": ["java", "kotlin", "scala", "groovy", "clojure"],
            "Script": ["python", "ruby", "lua", "perl", "r", "julia", "matlab", "octave", "gnuplot"],
            "Mobile": ["swift", "dart", "objective-c", "gdscript"],
            "Config": ["dockerfile", "dockercompose", "terraform", "nginx", "apache", "systemd", "makefile", "cmake", "gradle", "maven", "sbt", "leiningen", "mix", "cargo", "composer", "gemfile", "package.json"],
            "Data": ["sql", "graphql", "csv", "json", "xml", "yaml", "toml"],
            "Docs": ["markdown", "latex", "bibtex", "org", "rmd", "qmd"],
            "Legacy": ["pascal", "delphi", "ada", "fortran", "cobol", "lisp", "scheme", "racket", "prolog", "erlang", "elixir", "elm", "purescript"]
        }
        for family, langs in families.items():
            submenu = tk.Menu(parent_menu, tearoff=0, **self.menu_opts())
            parent_menu.add_cascade(label=family, menu=submenu)
            for name, lid, ext in LANGS:
                if lid in langs:
                    submenu.add_command(label=f"{name}  ({ext})", command=lambda l=lid: self.set_lexer(l))
        parent_menu.add_separator()
        parent_menu.add_command(label="Auto-détection", command=lambda: self.set_lexer("auto"))

    # -----------------------------------------------------------------------
    #  Barre d'outils
    # -----------------------------------------------------------------------
    def build_toolbar(self):
        t = THEMES[self.theme]
        bar = ttk.Frame(self, style="Material.TFrame")
        bar.pack(fill="x", padx=5, pady=5)
        icons = [("📄", self.new_file), ("📂", self.open_file), ("💾", self.save_file), ("✂", self.cut), ("📋", self.copy), ("📄", self.paste), ("🔍", self.show_find), ("⚙️", self.show_options_dialog)]
        for ic, cmd in icons:
            btn = MaterialButton(bar, text=ic, command=cmd, width=3)
            btn.pack(side="left", padx=2)
            self.add_tooltip(btn, ic)

    def add_tooltip(self, widget, text):
        """Attach a simple tooltip to a widget."""
        def on_enter(event):
            try:
                x = widget.winfo_rootx() + widget.winfo_width() // 2
                y = widget.winfo_rooty() + widget.winfo_height() + 6
                self._tooltip = tk.Toplevel(self)
                self._tooltip.wm_overrideredirect(True)
                self._tooltip.wm_geometry(f"+{x}+{y}")
                lbl = ttk.Label(self._tooltip, text=text, background=THEMES[self.theme]["menu_bg"], foreground=THEMES[self.theme]["menu_fg"], relief="solid", borderwidth=1)
                lbl.pack(ipadx=4, ipady=2)
            except Exception:
                pass

        def on_leave(event):
            try:
                if hasattr(self, "_tooltip") and self._tooltip:
                    self._tooltip.destroy()
                    self._tooltip = None
            except Exception:
                pass

        widget.bind("<Enter>", on_enter)
        widget.bind("<Leave>", on_leave)

    # -----------------------------------------------------------------------
    #  Zone principale (PanedWindow : explorateur | éditeur | minimap)
    # -----------------------------------------------------------------------
    def build_paned(self):
        t = THEMES[self.theme]
        self.paned = ttk.PanedWindow(self, orient="horizontal", style="Material.TFrame")
        self.paned.pack(fill="both", expand=True, padx=5, pady=5)

        # Explorateur (tree)
        self.explorer_frame = ttk.Frame(self.paned, style="Material.TFrame")
        self.build_explorer(self.explorer_frame)
        self.paned.add(self.explorer_frame, weight=0)

        # Éditeur
        self.editor_frame = ttk.Frame(self.paned, style="Material.TFrame")
        self.build_editor(self.editor_frame)
        self.paned.add(self.editor_frame, weight=1)

        # Minimap (placeholder)
        self.minimap_frame = ttk.Frame(self.paned, style="Material.TFrame")
        self.paned.add(self.minimap_frame, weight=0)

    def build_explorer(self, parent):
        t = THEMES[self.theme]
        lbl = ttk.Label(parent, text="EXPLORER", style="Material.TLabel", font=(self.cfg["font_family"], 9, "bold"))
        lbl.pack(fill="x", padx=5, pady=2)
        self.tree = ttk.Treeview(parent, height=20, show="tree", selectmode="browse")
        self.tree.pack(fill="both", expand=True, padx=5, pady=5)
        self.tree.bind("<Double-1>", self.on_tree_open)

    def on_tree_open(self, event):
        """Handle double-click in the explorer tree. If the node text is a file path, try to open it; otherwise show info."""
        try:
            sel = self.tree.selection()
            if not sel:
                return
            node = sel[0]
            node_text = self.tree.item(node, "text")
            # If it looks like a file path, try to open it
            if os.path.exists(node_text) and os.path.isfile(node_text):
                self.open_file(path=node_text)
            else:
                messagebox.showinfo("Explorer", f"Selected: {node_text}")
        except Exception as e:
            messagebox.showerror("Explorer error", str(e))

    def build_editor(self, parent):
        t = THEMES[self.theme]
        # Numéros de ligne
        self.line_canvas = tk.Canvas(parent, width=60, bg=t["margin_bg"], highlightthickness=0)
        self.line_canvas.pack(side="left", fill="y")
        # Text widget
        self.text = MaterialText(parent, wrap="word" if self.cfg["word_wrap"] else "none", font=(self.cfg["font_family"], self.cfg["font_size"]), bg=t["text_bg"], fg=t["text_fg"], insertbackground=t["text_fg"], selectbackground=t["select_bg"], selectforeground=t["select_fg"])
        self.text.pack(side="left", fill="both", expand=True)
        # Scrollbars
        vsb = ttk.Scrollbar(parent, orient="vertical", command=self.text.yview)
        vsb.pack(side="right", fill="y")
        self.text.configure(yscrollcommand=vsb.set)
        if not self.cfg["word_wrap"]:
            hsb = ttk.Scrollbar(parent, orient="horizontal", command=self.text.xview)
            hsb.pack(side="bottom", fill="x")
            self.text.configure(xscrollcommand=hsb.set)
        # Events
        self.text.bind("<KeyRelease>", self.on_key)
        self.text.bind("<<Modified>>", self.on_modified)
        self.text.bind("<Button-1>", self.on_click)
        self.text.bind("<Motion>", self.on_motion)

    # -----------------------------------------------------------------------
    #  Status bar
    # -----------------------------------------------------------------------
    def build_status(self):
        t = THEMES[self.theme]
        self.status = ttk.Frame(self, style="Material.TFrame")
        self.status.pack(side="bottom", fill="x")
        self.status_label = ttk.Label(self.status, text="Ready", style="Material.TLabel")
        self.status_label.pack(side="left", padx=10)
        self.pos_label = ttk.Label(self.status, text="L:1 C:1", style="Material.TLabel")
        self.pos_label.pack(side="right", padx=10)
        self.lang_label = ttk.Label(self.status, text="Auto", style="Material.TLabel")
        self.lang_label.pack(side="right", padx=10)

    # -----------------------------------------------------------------------
    #  Thème
    # -----------------------------------------------------------------------
    def apply_theme_to_text(self):
        t = THEMES[self.theme]
        self.text.configure(bg=t["text_bg"], fg=t["text_fg"], insertbackground=t["text_fg"], selectbackground=t["select_bg"], selectforeground=t["select_fg"])
        self.line_canvas.configure(bg=t["margin_bg"])
        for tag, color in self.get_syntax_colors().items():
            self.text.tag_configure(tag, foreground=color)

    def get_syntax_colors(self):
        t = THEMES[self.theme]
        return {"keyword": "#569CD6" if self.theme=="dark" else "#0000FF", "builtin": "#4EC9B0" if self.theme=="dark" else "#795EAC", "string": "#CE9178" if self.theme=="dark" else "#A31515", "comment": "#6A9955" if self.theme=="dark" else "#008000", "number": "#B5CEA8" if self.theme=="dark" else "#098658", "operator": "#D4D4D4" if self.theme=="dark" else "#000000", "function": "#DCDCAA" if self.theme=="dark" else "#795EAC", "class": "#4EC9B0" if self.theme=="dark" else "#267F99", "decorator": "#DCDCAA" if self.theme=="dark" else "#795EAC", "find": t["find_bg"]}

    # -----------------------------------------------------------------------
    #  Actions fichier
    # -----------------------------------------------------------------------
    def new_file(self):
        if self.check_unsaved():
            self.text.delete("1.0", "end")
            self.current_file = None
            self.modified = False
            self.update_title()
            self.set_lexer("auto")

    def open_file(self, path=None):
        if not self.check_unsaved():
            return
        if path is None:
            path = filedialog.askopenfilename(title=LANG[self.language]["open"], filetypes=[("All files", "*.*")])
        if path:
            try:
                with open(path, "r", encoding="utf-8") as f:
                    content = f.read()
                self.text.delete("1.0", "end")
                self.text.insert("1.0", content)
                self.current_file = path
                self.modified = False
                self.update_title()
                self.add_recent(path)
                self.set_lexer_by_ext(path)
                self.status_label.config(text=f"Opened {os.path.basename(path)}")
            except UnicodeDecodeError:
                messagebox.showerror("Encoding error", f"Cannot read {path} as UTF-8")
            except Exception as e:
                messagebox.showerror("Error", str(e))

    def save_file(self):
        if self.current_file:
            try:
                with open(self.current_file, "w", encoding="utf-8") as f:
                    f.write(self.text.get("1.0", "end-1c"))
                self.modified = False
                self.update_title()
                self.status_label.config(text=f"Saved {os.path.basename(self.current_file)}")
                return True
            except Exception as e:
                messagebox.showerror("Error", str(e))
                return False
        else:
            return self.save_as_file()

    def save_as_file(self):
        path = filedialog.asksaveasfilename(title=LANG[self.language]["save_as"], defaultextension=".txt", filetypes=[("All files", "*.*")])
        if path:
            self.current_file = path
            return self.save_file()
        return False

    def add_recent(self, path):
        if path in self.recent_files:
            self.recent_files.remove(path)
        self.recent_files.insert(0, path)
        self.recent_files = self.recent_files[:15]
        self.save_cfg()
        self.update_recent_menu()

    def update_recent_menu(self):
        self.recent_menu.delete(0, "end")
        if self.recent_files:
            for f in self.recent_files:
                self.recent_menu.add_command(label=os.path.basename(f), command=lambda p=f: self.open_file(p))
        else:
            self.recent_menu.add_command(label="No recent files", state="disabled")

    # -----------------------------------------------------------------------
    #  Lexer (coloration)
    # -----------------------------------------------------------------------
    def set_lexer(self, lid):
        self.current_lexer = lid
        self.cfg["current_lexer"] = lid
        self.lang_label.config(text=lid.capitalize())
        self.highlight_syntax()

    def set_lexer_by_ext(self, path):
        ext = pathlib.Path(path).suffix.lower()
        for name, lid, extp in LANGS:
            if ext == extp:
                self.set_lexer(lid)
                return
        self.set_lexer("auto")

    def highlight_syntax(self):
        if not self.syntax_var.get():
            return
        for tag in self.get_syntax_colors():
            self.text.tag_remove(tag, "1.0", "end")
        if self.current_lexer == "auto":
            if self.current_file:
                self.set_lexer_by_ext(self.current_file)
            return
        # Simple regex-based highlighting
        if self.current_lexer == "python":
            self.highlight_python()
        elif self.current_lexer in ("javascript", "typescript", "javascriptreact", "typescriptreact"):
            self.highlight_js()
        elif self.current_lexer == "html":
            self.highlight_html()
        elif self.current_lexer in ("css", "scss", "sass", "less", "stylus"):
            self.highlight_css()
        elif self.current_lexer in ("json", "jsonc", "json5"):
            self.highlight_json()
        elif self.current_lexer == "markdown":
            self.highlight_markdown()
        else:
            # Generic
            self.highlight_generic()

    def highlight_python(self):
        self.highlight_regex(r"\b(def|class|import|from|if|elif|else|for|while|try|except|finally|with|return|yield|lambda|and|or|not|in|is|True|False|None|async|await|nonlocal|global|assert|break|continue|pass|raise|del)\b", "keyword")
        self.highlight_regex(r"#.*$", "comment", re.MULTILINE)
        self.highlight_regex(r"('''.*?''')|(\"\"\".*?\"\"\")", "string", re.DOTALL)
        self.highlight_regex(r"('.*?')|(\".*?\")", "string")
        self.highlight_regex(r"\b\d+\.?\d*\b", "number")
        self.highlight_regex(r"\b[A-Za-z_]\w*(?=\()", "function")
        self.highlight_regex(r"@\w+", "decorator")

    def highlight_js(self):
        self.highlight_regex(r"\b(function|const|let|var|if|else|for|while|do|switch|case|default|break|continue|return|try|catch|finally|throw|new|this|typeof|instanceof|in|of|class|extends|super|import|export|from|async|await|yield|static|get|set)\b", "keyword")
        self.highlight_regex(r"//.*$", "comment", re.MULTILINE)
        self.highlight_regex(r"/\*.*?\*/", "comment", re.DOTALL)
        self.highlight_regex(r"('.*?')|(\".*?\")|(`.*?`)", "string")
        self.highlight_regex(r"\b\d+\.?\d*\b", "number")
        self.highlight_regex(r"\b[A-Za-z_]\w*(?=\()", "function")

    def highlight_html(self):
        self.highlight_regex(r"<[^>]+>", "keyword")
        self.highlight_regex(r"&\w+;", "builtin")

    def highlight_css(self):
        self.highlight_regex(r"[.#]?\w+\s*{", "keyword")
        self.highlight_regex(r"\w+:", "builtin")
        self.highlight_regex(r":[^;]+;", "string")
        self.highlight_regex(r"/\*.*?\*/", "comment", re.DOTALL)

    def highlight_json(self):
        self.highlight_regex(r"\".*?\"", "string")
        self.highlight_regex(r"\b(true|false|null)\b", "keyword")
        self.highlight_regex(r"\b\d+\.?\d*\b", "number")

    def highlight_markdown(self):
        self.highlight_regex(r"^#{1,6}\s+.*$", "keyword", re.MULTILINE)
        self.highlight_regex(r"\*\*.*?\*\*", "builtin")
        self.highlight_regex(r"\*.*?\*", "string")
        self.highlight_regex(r"`[^`]+`", "number")

    def highlight_generic(self):
        self.highlight_regex(r"('.*?')|(\".*?\")", "string")
        self.highlight_regex(r"\b\d+\.?\d*\b", "number")
        self.highlight_regex(r"#.*$", "comment", re.MULTILINE)

    def highlight_regex(self, pattern, tag, flags=0):
        content = self.text.get("1.0", "end-1c")
        for match in re.finditer(pattern, content, flags):
            start = f"1.0 + {match.start()} chars"
            end = f"1.0 + {match.end()} chars"
            self.text.tag_add(tag, start, end)

    # -----------------------------------------------------------------------
    #  Édition
    # -----------------------------------------------------------------------
    def undo(self): self.text.event_generate("<<Undo>>")
    def redo(self): self.text.event_generate("<<Redo>>")
    def cut(self): self.text.event_generate("<<Cut>>")
    def copy(self): self.text.event_generate("<<Copy>>")
    def paste(self): self.text.event_generate("<<Paste>>")
    def select_all(self):
        self.text.tag_add("sel", "1.0", "end")
        self.text.mark_set("insert", "1.0")
        self.text.see("insert")

    # -----------------------------------------------------------------------
    #  Recherche / Remplacement
    # -----------------------------------------------------------------------
    def show_find(self):
        FindReplaceDialog(self, mode="find")

    def show_replace(self):
        FindReplaceDialog(self, mode="replace")

    def goto_line(self):
        line = simpledialog.askinteger(LANG[self.language]["goto"], LANG[self.language]["goto"] + ":", minvalue=1, maxvalue=10000)
        if line:
            self.text.mark_set("insert", f"{line}.0")
            self.text.see(f"{line}.0")

    # -----------------------------------------------------------------------
    #  Options (dialogue géant)
    # -----------------------------------------------------------------------
    def show_options_dialog(self):
        OptionsDialog(self)

    # -----------------------------------------------------------------------
    #  Vue
    # -----------------------------------------------------------------------
    def toggle_wrap(self):
        self.cfg["word_wrap"] = self.wrap_var.get()
        self.text.configure(wrap="word" if self.cfg["word_wrap"] else "none")
        self.save_cfg()

    def toggle_line_numbers(self):
        self.cfg["show_line_numbers"] = self.num_var.get()
        if self.cfg["show_line_numbers"]:
            self.line_canvas.pack(side="left", fill="y")
        else:
            self.line_canvas.pack_forget()
        self.save_cfg()

    def toggle_syntax(self):
        self.highlight_syntax()

    def toggle_theme(self):
        self.theme = "dark" if self.theme == "light" else "light"
        self.cfg["theme"] = self.theme
        self.setup_style()
        self.apply_theme_to_text()
        self.save_cfg()

    def zoom_in(self):
        self.cfg["font_size"] += 1
        self.text.configure(font=(self.cfg["font_family"], self.cfg["font_size"]))
        self.save_cfg()

    def zoom_out(self):
        if self.cfg["font_size"] > 6:
            self.cfg["font_size"] -= 1
            self.text.configure(font=(self.cfg["font_family"], self.cfg["font_size"]))
            self.save_cfg()

    # -----------------------------------------------------------------------
    #  Événements
    # -----------------------------------------------------------------------
    def on_key(self, event=None):
        self.update_line_numbers()
        self.highlight_syntax()
        self.update_status()

    def on_modified(self, event=None):
        if self.text.edit_modified():
            self.modified = True
            self.update_title()
            self.text.edit_modified(False)

    def on_click(self, event=None):
        self.update_line_numbers()
        self.update_status()

    def on_motion(self, event=None):
        self.update_status()

    def update_line_numbers(self):
        if not self.cfg["show_line_numbers"]:
            return
        self.line_canvas.delete("all")
        first = self.text.index("@0,0")
        last = self.text.index(f"@0,{self.text.winfo_height()}")
        first_line = int(first.split(".")[0])
        last_line = int(last.split(".")[0]) + 1
        for line in range(first_line, last_line + 1):
            pos = self.text.bbox(f"{line}.0")
            if pos:
                y = pos[1] + (pos[3] - pos[1]) // 2
                self.line_canvas.create_text(50, y, text=str(line), anchor="ne", fill=THEMES[self.theme]["margin_fg"], font=(self.cfg["font_family"], self.cfg["font_size"] - 2))

    def update_status(self):
        line, col = self.text.index("insert").split(".")
        self.pos_label.config(text=f"L:{line} C:{col}")
        self.status_label.config(text=self.current_lexer.capitalize())

    def update_title(self):
        name = os.path.basename(self.current_file) if self.current_file else "Untitled"
        mod = " ●" if self.modified else ""
        self.title(f"{mod}{name} – Colony - Mother Sphere Text Editor")

    def check_unsaved(self):
        if self.modified:
            ans = messagebox.askyesnocancel("Unsaved changes", "Save changes before closing?")
            if ans is True:
                return self.save_file()
            elif ans is False:
                return True
            else:
                return False
        return True

    # -----------------------------------------------------------------------
    #  Raccourcis
    # -----------------------------------------------------------------------
    def bind_all_shortcuts(self):
        self.bind("<Control-n>", lambda e: self.new_file())
        self.bind("<Control-o>", lambda e: self.open_file())
        self.bind("<Control-s>", lambda e: self.save_file())
        self.bind("<Control-Shift-S>", lambda e: self.save_as_file())
        self.bind("<Control-z>", lambda e: self.undo())
        self.bind("<Control-y>", lambda e: self.redo())
        self.bind("<Control-f>", lambda e: self.show_find())
        self.bind("<Control-h>", lambda e: self.show_replace())
        self.bind("<Control-g>", lambda e: self.goto_line())
        self.bind("<Control-t>", lambda e: self.toggle_theme())
        self.bind("<Control-plus>", lambda e: self.zoom_in())
        self.bind("<Control-minus>", lambda e: self.zoom_out())
        self.bind("<F1>", lambda e: self.show_about())
        self.bind("<Control-Shift-P>", lambda e: self.show_command_palette())

    # -----------------------------------------------------------------------
    #  Command Palette
    # -----------------------------------------------------------------------
    def show_command_palette(self):
        PaletteDialog(self)

    # -----------------------------------------------------------------------
    #  About
    # -----------------------------------------------------------------------
    def show_about(self):
        messagebox.showinfo("About", "Colony\nAll-in-one text editor\nMaterial Design 3\n180+ languages\n200+ settings\nBuilt with Python & Tkinter")

    # -----------------------------------------------------------------------
    #  Exit
    # -----------------------------------------------------------------------
    def on_exit(self):
        if self.check_unsaved():
            self.save_cfg()
            self.destroy()

# ---------------------------------------------------------------------------
#  Dialogues utilitaires
# ---------------------------------------------------------------------------
class FindReplaceDialog(tk.Toplevel):
    def __init__(self, parent, mode="find"):
        super().__init__(parent)
        self.parent = parent
        self.mode = mode
        self.title("Find" if mode=="find" else "Replace")
        self.transient(parent)
        self.geometry("400x200")
        self.configure(bg=THEMES[parent.theme]["bg"])
        self.build()
        self.search_term = ""
        self.replace_term = ""

    def build(self):
        t = THEMES[self.parent.theme]
        main = ttk.Frame(self, style="Material.TFrame")
        main.pack(fill="both", expand=True, padx=20, pady=20)
        ttk.Label(main, text="Find:", style="Material.TLabel").grid(row=0, column=0, sticky="w", pady=5)
        self.find_entry = ttk.Entry(main, width=30, style="Material.TEntry")
        self.find_entry.grid(row=0, column=1, padx=10, pady=5)
        self.find_entry.focus()
        if self.mode == "replace":
            ttk.Label(main, text="Replace with:", style="Material.TLabel").grid(row=1, column=0, sticky="w", pady=5)
            self.replace_entry = ttk.Entry(main, width=30, style="Material.TEntry")
            self.replace_entry.grid(row=1, column=1, padx=10, pady=5)
        btn_frame = ttk.Frame(main, style="Material.TFrame")
        btn_frame.grid(row=2, column=0, columnspan=2, pady=20)
        MaterialButton(btn_frame, text="Find Next", command=self.find_next).pack(side="left", padx=5)
        if self.mode == "replace":
            MaterialButton(btn_frame, text="Replace", command=self.replace_one).pack(side="left", padx=5)
            MaterialButton(btn_frame, text="Replace All", command=self.replace_all).pack(side="left", padx=5)
        MaterialButton(btn_frame, text="Close", command=self.destroy).pack(side="left", padx=5)
        self.find_entry.bind("<Return>", lambda e: self.find_next())

    def find_next(self):
        self.search_term = self.find_entry.get()
        if not self.search_term:
            return
        pos = self.parent.text.search(self.search_term, "insert", "end", nocase=True)
        if pos:
            end = f"{pos}+{len(self.search_term)}c"
            self.parent.text.tag_remove("find", "1.0", "end")
            self.parent.text.tag_add("find", pos, end)
            self.parent.text.tag_configure("find", background=THEMES[self.parent.theme]["find_bg"])
            self.parent.text.mark_set("insert", end)
            self.parent.text.see(pos)
        else:
            messagebox.showinfo("Find", "No more occurrences")

    def replace_one(self):
        self.search_term = self.find_entry.get()
        self.replace_term = self.replace_entry.get()
        if not self.search_term:
            return
        sel = self.parent.text.tag_ranges("sel")
        if sel and self.parent.text.get(*sel) == self.search_term:
            self.parent.text.delete(*sel)
            self.parent.text.insert(sel[0], self.replace_term)
        self.find_next()

    def replace_all(self):
        self.search_term = self.find_entry.get()
        self.replace_term = self.replace_entry.get()
        if not self.search_term:
            return
        content = self.parent.text.get("1.0", "end-1c")
        new_content = content.replace(self.search_term, self.replace_term)
        if new_content != content:
            self.parent.text.delete("1.0", "end")
            self.parent.text.insert("1.0", new_content)
            messagebox.showinfo("Replace", f"Replaced {content.count(self.search_term)} occurrences")

class PaletteDialog(tk.Toplevel):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.title("Command Palette")
        self.transient(parent)
        self.geometry("600x400")
        self.configure(bg=THEMES[parent.theme]["bg"])
        self.build()
        self.commands = self.build_commands()
        self.update_list("")

    def build(self):
        t = THEMES[self.parent.theme]
        main = ttk.Frame(self, style="Material.TFrame")
        main.pack(fill="both", expand=True, padx=20, pady=20)
        ttk.Label(main, text="Type a command:", style="Material.TLabel").pack(anchor="w")
        self.entry = ttk.Entry(main, width=60, style="Material.TEntry")
        self.entry.pack(fill="x", pady=5)
        self.entry.focus()
        self.entry.bind("<KeyRelease>", self.on_key)
        self.listbox = tk.Listbox(main, bg=t["text_bg"], fg=t["text_fg"], selectbackground=t["select_bg"], selectforeground=t["select_fg"], height=15)
        self.listbox.pack(fill="both", expand=True, pady=5)
        self.listbox.bind("<Double-1>", self.on_run)
        self.entry.bind("<Return>", self.on_run)

    def build_commands(self):
        return [
            ("New File", self.parent.new_file),
            ("Open File", self.parent.open_file),
            ("Save File", self.parent.save_file),
            ("Save As", self.parent.save_as_file),
            ("Undo", self.parent.undo),
            ("Redo", self.parent.redo),
            ("Cut", self.parent.cut),
            ("Copy", self.parent.copy),
            ("Paste", self.parent.paste),
            ("Find", self.parent.show_find),
            ("Replace", self.parent.show_replace),
            ("Go to Line", self.parent.goto_line),
            ("Zoom In", self.parent.zoom_in),
            ("Zoom Out", self.parent.zoom_out),
            ("Toggle Word Wrap", lambda: self.parent.wrap_var.set(not self.parent.wrap_var.get()) or self.parent.toggle_wrap()),
            ("Toggle Line Numbers", lambda: self.parent.num_var.set(not self.parent.num_var.get()) or self.parent.toggle_line_numbers()),
            ("Toggle Theme", self.parent.toggle_theme),
            ("Options", self.parent.show_options_dialog),
            ("About", self.parent.show_about),
            ("Quit", self.parent.on_exit),
        ]

    def update_list(self, filter_text):
        self.listbox.delete(0, "end")
        filter_lower = filter_text.lower()
        for name, cmd in self.commands:
            if filter_lower in name.lower():
                self.listbox.insert("end", name)

    def on_key(self, event=None):
        self.update_list(self.entry.get())

    def on_run(self, event=None):
        selection = self.listbox.curselection()
        if selection:
            name = self.listbox.get(selection[0])
            for cmd_name, cmd in self.commands:
                if cmd_name == name:
                    self.destroy()
                    cmd()
                    return

class OptionsDialog(tk.Toplevel):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.title(LANG[parent.language]["options"])
        self.transient(parent)
        self.geometry("900x700")
        self.configure(bg=THEMES[parent.theme]["bg"])
        self.build()
        self.load_values()

    def build(self):
        t = THEMES[self.parent.theme]
        main = ttk.Frame(self, style="Material.TFrame")
        main.pack(fill="both", expand=True, padx=20, pady=20)
        # Search bar
        search_frame = ttk.Frame(main, style="Material.TFrame")
        search_frame.pack(fill="x", pady=5)
        ttk.Label(search_frame, text=LANG[self.parent.language]["search_settings"], style="Material.TLabel").pack(side="left", padx=5)
        self.search_entry = ttk.Entry(search_frame, width=40, style="Material.TEntry")
        self.search_entry.pack(side="left", padx=5)
        self.search_entry.bind("<KeyRelease>", self.on_search)
        # Notebook
        self.nb = ttk.Notebook(main)
        self.nb.pack(fill="both", expand=True, pady=10)
        self.build_editor_tab()
        self.build_appearance_tab()
        self.build_behavior_tab()
        self.build_language_tab()
        self.build_shortcuts_tab()
        self.build_tools_tab()
        self.build_git_tab()
        self.build_extensions_tab()
        # Buttons
        btn_frame = ttk.Frame(main, style="Material.TFrame")
        btn_frame.pack(fill="x", pady=10)
        MaterialButton(btn_frame, text=LANG[self.parent.language]["restore_defaults"], command=self.restore_defaults).pack(side="left", padx=5)
        MaterialButton(btn_frame, text=LANG[self.parent.language]["import_settings"], command=self.import_settings).pack(side="left", padx=5)
        MaterialButton(btn_frame, text=LANG[self.parent.language]["export_settings"], command=self.export_settings).pack(side="left", padx=5)
        MaterialButton(btn_frame, text=LANG[self.parent.language]["apply"], command=self.apply).pack(side="right", padx=5)
        MaterialButton(btn_frame, text=LANG[self.parent.language]["close"], command=self.destroy).pack(side="right", padx=5)

    def build_editor_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["editor"])
        # Font
        row = 0
        ttk.Label(tab, text="Font family:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.font_family_var = tk.StringVar(value=self.parent.cfg["font_family"])
        ttk.Entry(tab, textvariable=self.font_family_var, width=30, style="Material.TEntry").grid(row=row, column=1, padx=10, pady=5)
        row += 1
        ttk.Label(tab, text="Font size:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.font_size_var = tk.IntVar(value=self.parent.cfg["font_size"])
        ttk.Spinbox(tab, from_=6, to=72, textvariable=self.font_size_var, width=10, style="Material.TSpinbox").grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Tab size
        row += 1
        ttk.Label(tab, text="Tab size:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.tab_size_var = tk.IntVar(value=self.parent.cfg["tab_size"])
        ttk.Spinbox(tab, from_=1, to=16, textvariable=self.tab_size_var, width=10, style="Material.TSpinbox").grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Checkboxes
        row += 1
        self.word_wrap_var = tk.BooleanVar(value=self.parent.cfg["word_wrap"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["word_wrap"], variable=self.word_wrap_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.line_numbers_var = tk.BooleanVar(value=self.parent.cfg["show_line_numbers"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["line_numbers"], variable=self.line_numbers_var, style="Material.TCheckbutton").grid(row=row, column=1, sticky="w", padx=10, pady=5)
        row += 1
        self.whitespace_var = tk.BooleanVar(value=self.parent.cfg["show_whitespace"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["show_whitespace"], variable=self.whitespace_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.spaces_var = tk.BooleanVar(value=self.parent.cfg["indent_with_spaces"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["indent_with_spaces"], variable=self.spaces_var, style="Material.TCheckbutton").grid(row=row, column=1, sticky="w", padx=10, pady=5)
        row += 1
        self.trim_var = tk.BooleanVar(value=self.parent.cfg["trim_trailing"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["trim_trailing"], variable=self.trim_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.final_newline_var = tk.BooleanVar(value=self.parent.cfg["insert_final_newline"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["insert_final_newline"], variable=self.final_newline_var, style="Material.TCheckbutton").grid(row=row, column=1, sticky="w", padx=10, pady=5)
        row += 1
        self.format_var = tk.BooleanVar(value=self.parent.cfg["format_on_save"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["format_on_save"], variable=self.format_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.auto_save_var = tk.BooleanVar(value=self.parent.cfg["auto_save"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["auto_save"], variable=self.auto_save_var, style="Material.TCheckbutton").grid(row=row, column=1, sticky="w", padx=10, pady=5)
        row += 1
        ttk.Label(tab, text="Auto-save delay (ms):", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.auto_save_delay_var = tk.IntVar(value=self.parent.cfg["auto_save_delay"])
        ttk.Spinbox(tab, from_=1000, to=600000, textvariable=self.auto_save_delay_var, width=10, style="Material.TSpinbox").grid(row=row, column=1, padx=10, pady=5, sticky="w")

    def build_appearance_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["appearance"])
        # Theme
        row = 0
        ttk.Label(tab, text="Theme:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.theme_var = tk.StringVar(value=self.parent.theme)
        ttk.Combobox(tab, textvariable=self.theme_var, values=["light", "dark"], state="readonly", width=20, style="Material.TCombobox").grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Minimap
        row += 1
        self.minimap_var = tk.BooleanVar(value=self.parent.cfg["minimap"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["minimap"], variable=self.minimap_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        # Breadcrumb
        row += 1
        self.breadcrumb_var = tk.BooleanVar(value=self.parent.cfg["breadcrumb"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["breadcrumb"], variable=self.breadcrumb_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        # Sticky scroll
        row += 1
        self.sticky_var = tk.BooleanVar(value=self.parent.cfg["sticky_scroll"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["sticky_scroll"], variable=self.sticky_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.inlay_var = tk.BooleanVar(value=self.parent.cfg["inlay_hints"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["inlay_hints"], variable=self.inlay_var, style="Material.TCheckbutton").grid(row=row, column=1, sticky="w", padx=10, pady=5)
        # Bracket pairs
        row += 1
        self.bracket_var = tk.BooleanVar(value=self.parent.cfg["bracket_pairs"])
        ttk.Checkbutton(tab, text=LANG[self.parent.language]["bracket_pairs"], variable=self.bracket_var, style="Material.TCheckbutton").grid(row=row, column=0, sticky="w", padx=10, pady=5)

    def build_behavior_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["behavior"])
        # rulers
        row = 0
        ttk.Label(tab, text="Rulers (comma separated):", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.rulers_var = tk.StringVar(value=",".join(map(str, self.parent.cfg["rulers"])))
        ttk.Entry(tab, textvariable=self.rulers_var, width=30, style="Material.TEntry").grid(row=row, column=1, padx=10, pady=5)
        # terminal shell
        row += 1
        ttk.Label(tab, text="Terminal shell:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.shell_var = tk.StringVar(value=self.parent.cfg["terminal_shell"])
        ttk.Entry(tab, textvariable=self.shell_var, width=30, style="Material.TEntry").grid(row=row, column=1, padx=10, pady=5)
        # terminal font
        row += 1
        ttk.Label(tab, text="Terminal font:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.term_font_var = tk.StringVar(value=self.parent.cfg["terminal_font"])
        ttk.Entry(tab, textvariable=self.term_font_var, width=30, style="Material.TEntry").grid(row=row, column=1, padx=10, pady=5)

    def build_language_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["languages"])
        row = 0
        ttk.Label(tab, text="Interface language:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.lang_var = tk.StringVar(value=self.parent.language)
        ttk.Combobox(tab, textvariable=self.lang_var, values=["fr", "en", "zh"], state="readonly", width=20, style="Material.TCombobox").grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Current lexer
        row += 1
        ttk.Label(tab, text="Default syntax highlighter:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.lexer_var = tk.StringVar(value=self.parent.current_lexer)
        ttk.Combobox(tab, textvariable=self.lexer_var, values=["auto"] + [lid for _, lid, _ in LANGS], state="readonly", width=20, style="Material.TCombobox").grid(row=row, column=1, padx=10, pady=5, sticky="w")

    def build_shortcuts_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["shortcuts"])
        # Liste des raccourcis
        frame = ttk.Frame(tab, style="Material.TFrame")
        frame.pack(fill="both", expand=True, padx=10, pady=10)
        columns = ("Command", "Shortcut")
        self.tree = ttk.Treeview(frame, columns=columns, show="headings", height=15, style="Treeview")
        for col in columns:
            self.tree.heading(col, text=col)
            self.tree.column(col, width=200)
        self.tree.pack(fill="both", expand=True)
        # Remplir avec des raccourcis par défaut
        shortcuts = [
            ("New File", "Ctrl+N"),
            ("Open File", "Ctrl+O"),
            ("Save File", "Ctrl+S"),
            ("Save As", "Ctrl+Shift+S"),
            ("Undo", "Ctrl+Z"),
            ("Redo", "Ctrl+Y"),
            ("Find", "Ctrl+F"),
            ("Replace", "Ctrl+H"),
            ("Go to Line", "Ctrl+G"),
            ("Toggle Theme", "Ctrl+T"),
            ("Zoom In", "Ctrl++"),
            ("Zoom Out", "Ctrl+-"),
            ("Command Palette", "Ctrl+Shift+P"),
        ]
        for cmd, shortcut in shortcuts:
            self.tree.insert("", "end", values=(cmd, shortcut))

    def build_tools_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text="Tools")
        # ASCII Art
        row = 0
        MaterialButton(tab, text=LANG[self.parent.language]["ascii_art"], command=self.ascii_art).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        # QR Code
        MaterialButton(tab, text=LANG[self.parent.language]["qr_code"], command=self.qr_code).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Base64
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["base64_encode"], command=self.base64_encode).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["base64_decode"], command=self.base64_decode).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Hash
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["hash_md5"], command=self.hash_md5).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["hash_sha256"], command=self.hash_sha256).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Timestamp / UUID
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["timestamp"], command=self.timestamp).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["uuid_gen"], command=self.uuid_gen).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Color picker
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["color_picker"], command=self.color_picker).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        # Emoji panel
        MaterialButton(tab, text=LANG[self.parent.language]["emoji_panel"], command=self.emoji_panel).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # LaTeX / PlantUML / Mermaid / Graphviz
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["latex_math"], command=self.latex_math).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["plantuml"], command=self.plantuml).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["mermaid"], command=self.mermaid).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["graphviz"], command=self.graphviz).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # CSV / JSON / XML / Diff
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["csv_viewer"], command=self.csv_viewer).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["json_tree"], command=self.json_tree).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["xml_tree"], command=self.xml_tree).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["diff_viewer"], command=self.diff_viewer).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        # Git
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["git_blame"], command=self.git_blame).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text=LANG[self.parent.language]["git_log"], command=self.git_log).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        row += 1
        MaterialButton(tab, text=LANG[self.parent.language]["git_graph"], command=self.git_graph).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        # System info
        MaterialButton(tab, text=LANG[self.parent.language]["system_info"], command=self.system_info).grid(row=row, column=1, padx=10, pady=5, sticky="w")

    def build_git_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["git"])
        # Git buttons
        row = 0
        MaterialButton(tab, text="Git Init", command=self.git_init).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text="Git Status", command=self.git_status).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        row += 1
        MaterialButton(tab, text="Git Add All", command=self.git_add_all).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text="Git Commit", command=self.git_commit).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        row += 1
        MaterialButton(tab, text="Git Push", command=self.git_push).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text="Git Pull", command=self.git_pull).grid(row=row, column=1, padx=10, pady=5, sticky="w")
        row += 1
        MaterialButton(tab, text="Git Log", command=self.git_log).grid(row=row, column=0, padx=10, pady=5, sticky="w")
        MaterialButton(tab, text="Git Graph", command=self.git_graph).grid(row=row, column=1, padx=10, pady=5, sticky="w")

    def build_extensions_tab(self):
        t = THEMES[self.parent.theme]
        tab = ttk.Frame(self.nb, style="Material.TFrame")
        self.nb.add(tab, text=LANG[self.parent.language]["extensions"])
        # Liste factice d’extensions
        row = 0
        ttk.Label(tab, text="Installed extensions:", style="Material.TLabel").grid(row=row, column=0, sticky="w", padx=10, pady=5)
        self.ext_listbox = tk.Listbox(tab, bg=t["text_bg"], fg=t["text_fg"], selectbackground=t["select_bg"], selectforeground=t["select_fg"], height=10)
        self.ext_listbox.grid(row=row+1, column=0, columnspan=2, sticky="nsew", padx=10, pady=5)
        ext_scroll = ttk.Scrollbar(tab, orient="vertical", command=self.ext_listbox.yview)
        ext_scroll.grid(row=row+1, column=2, sticky="ns")
        self.ext_listbox.configure(yscrollcommand=ext_scroll.set)
        # Extensions fictives
        for ext in ["Python", "Pylance", "GitLens", "Material Theme", "Prettier", "ESLint", "Bracket Pair Colorizer", "Auto Rename Tag", "Tailwind CSS IntelliSense"]:
            self.ext_listbox.insert("end", ext)
        # Boutons
        btn_frame = ttk.Frame(tab, style="Material.TFrame")
        btn_frame.grid(row=row+2, column=0, columnspan=3, pady=10)
        MaterialButton(btn_frame, text="Install", command=self.ext_install).pack(side="left", padx=5)
        MaterialButton(btn_frame, text="Uninstall", command=self.ext_uninstall).pack(side="left", padx=5)
        MaterialButton(btn_frame, text="Update", command=self.ext_update).pack(side="left", padx=5)
        MaterialButton(btn_frame, text="Reload", command=self.ext_reload).pack(side="left", padx=5)

    # -----------------------------------------------------------------------
    #  Méthodes outils
    # -----------------------------------------------------------------------
    def ascii_art(self):
        text = self.parent.text.get("1.0", "end-1c")[:200]
        if "pyfiglet" in globals():
            art = globals()["pyfiglet"].figlet_format(text)
        else:
            art = "Install pyfiglet for ASCII art"
        self.show_tool_output("ASCII Art", art)

    def qr_code(self):
        data = self.parent.text.get("1.0", "end-1c")[:200]
        if not data.strip():
            data = "https://github.com/MotherSphere/Colony"
        if qrcode is None:
            self.show_tool_output("QR Code", "Install 'qrcode' package to use this feature")
            return
        qr = qrcode.QRCode(border=1)
        qr.add_data(data)
        qr.make(fit=True)
        # Render QR matrix as text for display
        matrix_text = "\n".join("".join("██" if c else "  " for c in row) for row in qr.get_matrix())
        self.show_tool_output("QR Code", data + "\n\n" + matrix_text)

    def base64_encode(self):
        txt = self.parent.text.get("1.0", "end-1c")
        enc = base64.b64encode(txt.encode("utf-8")).decode("utf-8")
        self.show_tool_output("Base64 Encode", enc)

    def base64_decode(self):
        txt = self.parent.text.get("1.0", "end-1c")
        try:
            dec = base64.b64decode(txt.encode("utf-8")).decode("utf-8")
            self.show_tool_output("Base64 Decode", dec)
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def hash_md5(self):
        txt = self.parent.text.get("1.0", "end-1c")
        h = hashlib.md5(txt.encode("utf-8")).hexdigest()
        self.show_tool_output("MD5", h)

    def hash_sha256(self):
        txt = self.parent.text.get("1.0", "end-1c")
        h = hashlib.sha256(txt.encode("utf-8")).hexdigest()
        self.show_tool_output("SHA-256", h)

    def timestamp(self):
        now = datetime.datetime.now()
        unix = int(now.timestamp())
        iso = now.isoformat()
        self.show_tool_output("Timestamp", f"Unix: {unix}\nISO: {iso}")

    def uuid_gen(self):
        self.show_tool_output("UUID", str(uuid.uuid4()))

    def color_picker(self):
        color = colorchooser.askcolor(title="Pick a color")[1]
        if color:
            self.show_tool_output("Color", f"Hex: {color}\nRGB: {tuple(int(color[i:i+2], 16) for i in (1, 3, 5))}")

    def emoji_panel(self):
        emojis = "😀 😃 😄 😁 😆 😅 😂 🤣 🥲 ☺️ 😊 😇 🙂 🙃 😉 😌 😍 🥰 😘 😗 😙 😚 😋 😛 😝 😜 🤪 🤨 🧐 🤓 😎 🥸 🤩 🥳 😏 😒 😞 😔 😟 😕 🙁 ☹️ 😣 😖 😫 😩 🥺 😢 😭 😤 😠 😡 🤬 🤯 😳 🥵 🥶 😱 😨 😰 😥 😓 🤗 🤔 🤭 🤫 🤥 😶 😐 😑 😬 🙄 😯 😦 😧 😮 😲 🥱 😴 🤤 😪 😵 🤐 🥴 🤢 🤮 🤧 😷 🤒 🤕 🤑 🤠 😈 👿 👹 👺 🤡 💩 👻 💀 ☠️ 👽 👾 🤖 🎃 😺 😸 😹 😻 😼 😽 🙀 😿 😾"
        self.show_tool_output("Emoji Panel", emojis)

    def latex_math(self):
        formula = simpledialog.askstring("LaTeX Math", "Enter LaTeX formula:")
        if formula:
            self.show_tool_output("LaTeX", f"$$ {formula} $$")

    def plantuml(self):
        code = self.parent.text.get("1.0", "end-1c")
        if "@startuml" not in code:
            code = "@startuml\nAlice -> Bob: Hello\nBob -> Alice: Hi!\n@enduml"
        self.show_tool_output("PlantUML", code)

    def mermaid(self):
        code = self.parent.text.get("1.0", "end-1c")
        if "graph" not in code:
            code = "graph TD\nA[Start] --> B{Is it?}\nB -->|Yes| C[OK]\nB -->|No| D[Not OK]"
        self.show_tool_output("Mermaid", code)

    def graphviz(self):
        code = self.parent.text.get("1.0", "end-1c")
        if "digraph" not in code:
            code = "digraph G {\nA -> B\nB -> C\nC -> A\n}"
        self.show_tool_output("Graphviz", code)

    def csv_viewer(self):
        csv_data = self.parent.text.get("1.0", "end-1c")
        try:
            reader = csv.reader(csv_data.splitlines())
            lines = list(reader)
            out = "\n".join([" | ".join(row) for row in lines])
            self.show_tool_output("CSV Viewer", out)
        except Exception as e:
            messagebox.showerror("CSV Error", str(e))

    def json_tree(self):
        txt = self.parent.text.get("1.0", "end-1c")
        try:
            data = json.loads(txt)
            out = json.dumps(data, indent=2, ensure_ascii=False)
            self.show_tool_output("JSON Tree", out)
        except Exception as e:
            messagebox.showerror("JSON Error", str(e))

    def xml_tree(self):
        txt = self.parent.text.get("1.0", "end-1c")
        try:
            root = ET.fromstring(txt)
            out = ET.tostring(root, encoding="unicode", method="xml")
            self.show_tool_output("XML Tree", out)
        except Exception as e:
            messagebox.showerror("XML Error", str(e))

    def diff_viewer(self):
        txt = self.parent.text.get("1.0", "end-1c")
        lines = txt.splitlines()
        if len(lines) < 2:
            messagebox.showinfo("Diff", "Need at least two blocks separated by '---'")
            return
        try:
            sep = lines.index("---")
            a = lines[:sep]
            b = lines[sep+1:]
            diff = difflib.unified_diff(a, b, lineterm="")
            self.show_tool_output("Diff Viewer", "\n".join(diff))
        except ValueError:
            messagebox.showinfo("Diff", "Insert '---' between two versions")

    def git_blame(self):
        if not self.parent.current_file:
            messagebox.showwarning("Git", "No file opened")
            return
        try:
            result = subprocess.run(["git", "blame", self.parent.current_file], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            self.show_tool_output("Git Blame", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_log(self):
        try:
            result = subprocess.run(["git", "log", "--oneline", "-10"], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            self.show_tool_output("Git Log", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_graph(self):
        try:
            result = subprocess.run(["git", "log", "--graph", "--pretty=oneline", "--abbrev-commit", "-10"], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            self.show_tool_output("Git Graph", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_init(self):
        try:
            result = subprocess.run(["git", "init"], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            messagebox.showinfo("Git Init", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def system_info(self):
        """Show simple system information in the tools dialog."""
        try:
            info = []
            info.append(f"Platform: {platform.platform()}")
            info.append(f"System: {platform.system()} {platform.release()}")
            info.append(f"Python: {platform.python_version()}")
            info.append(f"Machine: {platform.machine()}")
            info.append(f"Processor: {platform.processor()}")
            info.append(f"CPU count: {os.cpu_count()}")
            info.append(f"Working dir: {os.getcwd()}")
            info.append(f"Missing optional libs: {', '.join(MISSING) if MISSING else 'None'}")
            self.show_tool_output("System Info", "\n".join(info))
        except Exception as e:
            messagebox.showerror("System Info", str(e))

    def git_status(self):
        try:
            result = subprocess.run(["git", "status"], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            self.show_tool_output("Git Status", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_add_all(self):
        try:
            result = subprocess.run(["git", "add", "."], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            messagebox.showinfo("Git Add", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_commit(self):
        msg = simpledialog.askstring("Git Commit", "Commit message:")
        if not msg:
            return
        try:
            result = subprocess.run(["git", "commit", "-m", msg], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            messagebox.showinfo("Git Commit", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_push(self):
        try:
            result = subprocess.run(["git", "push"], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            messagebox.showinfo("Git Push", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    def git_pull(self):
        try:
            result = subprocess.run(["git", "pull"], capture_output=True, text=True, cwd=os.path.dirname(self.parent.current_file) or ".")
            messagebox.showinfo("Git Pull", result.stdout or result.stderr)
        except FileNotFoundError:
            messagebox.showerror("Git", "Git not found in PATH")

    # Extensions factices
    def ext_install(self):
        sel = self.ext_listbox.curselection()
        if sel:
            ext = self.ext_listbox.get(sel[0])
            messagebox.showinfo("Extensions", f"Installed {ext}")
        else:
            messagebox.showwarning("Extensions", "Select an extension")

    def ext_uninstall(self):
        sel = self.ext_listbox.curselection()
        if sel:
            ext = self.ext_listbox.get(sel[0])
            messagebox.showinfo("Extensions", f"Uninstalled {ext}")
        else:
            messagebox.showwarning("Extensions", "Select an extension")

    def ext_update(self):
        messagebox.showinfo("Extensions", "All extensions updated")

    def ext_reload(self):
        messagebox.showinfo("Extensions", "Extensions reloaded")

    # -----------------------------------------------------------------------
    #  Utilitaires communs
    # -----------------------------------------------------------------------
    def show_tool_output(self, title, content):
        dlg = tk.Toplevel(self.parent)
        dlg.title(title)
        dlg.transient(self.parent)
        dlg.geometry("600x400")
        dlg.configure(bg=THEMES[self.parent.theme]["bg"])
        txt = ScrolledText(dlg, bg=THEMES[self.parent.theme]["text_bg"], fg=THEMES[self.parent.theme]["text_fg"], insertbackground=THEMES[self.parent.theme]["text_fg"], font=(self.parent.cfg["font_family"], self.parent.cfg["font_size"]))
        txt.pack(fill="both", expand=True, padx=10, pady=10)
        txt.insert("1.0", content)
        txt.configure(state="disabled")
        MaterialButton(dlg, text="Close", command=dlg.destroy).pack(pady=5)

    def apply(self):
        # Appliquer tous les changements
        self.parent.cfg["font_family"] = self.font_family_var.get()
        self.parent.cfg["font_size"] = self.font_size_var.get()
        self.parent.cfg["word_wrap"] = self.word_wrap_var.get()
        self.parent.cfg["show_line_numbers"] = self.line_numbers_var.get()
        self.parent.cfg["show_whitespace"] = self.whitespace_var.get()
        self.parent.cfg["indent_with_spaces"] = self.spaces_var.get()
        self.parent.cfg["tab_size"] = self.tab_size_var.get()
        self.parent.cfg["trim_trailing"] = self.trim_var.get()
        self.parent.cfg["insert_final_newline"] = self.final_newline_var.get()
        self.parent.cfg["format_on_save"] = self.format_var.get()
        self.parent.cfg["auto_save"] = self.auto_save_var.get()
        self.parent.cfg["auto_save_delay"] = self.auto_save_delay_var.get()
        self.parent.cfg["theme"] = self.theme_var.get()
        self.parent.cfg["minimap"] = self.minimap_var.get()
        self.parent.cfg["breadcrumb"] = self.breadcrumb_var.get()
        self.parent.cfg["sticky_scroll"] = self.sticky_var.get()
        self.parent.cfg["inlay_hints"] = self.inlay_var.get()
        self.parent.cfg["bracket_pairs"] = self.bracket_var.get()
        self.parent.cfg["rulers"] = [int(x.strip()) for x in self.rulers_var.get().split(",") if x.strip().isdigit()]
        self.parent.cfg["terminal_shell"] = self.shell_var.get()
        self.parent.cfg["terminal_font"] = self.term_font_var.get()
        self.parent.cfg["language"] = self.lang_var.get()
        self.parent.cfg["current_lexer"] = self.lexer_var.get()
        self.parent.save_cfg()
        # Appliquer immédiatement
        self.parent.theme = self.parent.cfg["theme"]
        self.parent.setup_style()
        self.parent.apply_theme_to_text()
        self.parent.text.configure(font=(self.parent.cfg["font_family"], self.parent.cfg["font_size"]))
        self.parent.toggle_wrap()
        self.parent.toggle_line_numbers()
        self.parent.highlight_syntax()
        messagebox.showinfo("Options", "Settings applied")

    def restore_defaults(self):
        for key in DEFAULT_CFG:
            self.parent.cfg[key] = DEFAULT_CFG[key]
        self.load_values()
        messagebox.showinfo("Options", "Defaults loaded – click Apply to validate")

    def import_settings(self):
        path = filedialog.askopenfilename(title="Import settings", filetypes=[("JSON", "*.json")])
        if path:
            try:
                with open(path, "r", encoding="utf-8") as f:
                    imported = json.load(f)
                self.parent.cfg.update(imported)
                self.load_values()
                messagebox.showinfo("Options", "Settings imported – click Apply to validate")
            except Exception as e:
                messagebox.showerror("Import error", str(e))

    def export_settings(self):
        path = filedialog.asksaveasfilename(title="Export settings", defaultextension=".json", filetypes=[("JSON", "*.json")])
        if path:
            try:
                with open(path, "w", encoding="utf-8") as f:
                    json.dump(self.parent.cfg, f, indent=2, ensure_ascii=False)
                messagebox.showinfo("Options", "Settings exported")
            except Exception as e:
                messagebox.showerror("Export error", str(e))

    def load_values(self):
        self.font_family_var.set(self.parent.cfg["font_family"])
        self.font_size_var.set(self.parent.cfg["font_size"])
        self.word_wrap_var.set(self.parent.cfg["word_wrap"])
        self.line_numbers_var.set(self.parent.cfg["show_line_numbers"])
        self.whitespace_var.set(self.parent.cfg["show_whitespace"])
        self.spaces_var.set(self.parent.cfg["indent_with_spaces"])
        self.tab_size_var.set(self.parent.cfg["tab_size"])
        self.trim_var.set(self.parent.cfg["trim_trailing"])
        self.final_newline_var.set(self.parent.cfg["insert_final_newline"])
        self.format_var.set(self.parent.cfg["format_on_save"])
        self.auto_save_var.set(self.parent.cfg["auto_save"])
        self.auto_save_delay_var.set(self.parent.cfg["auto_save_delay"])
        self.theme_var.set(self.parent.cfg["theme"])
        self.minimap_var.set(self.parent.cfg["minimap"])
        self.breadcrumb_var.set(self.parent.cfg["breadcrumb"])
        self.sticky_var.set(self.parent.cfg["sticky_scroll"])
        self.inlay_var.set(self.parent.cfg["inlay_hints"])
        self.bracket_var.set(self.parent.cfg["bracket_pairs"])
        self.rulers_var.set(",".join(map(str, self.parent.cfg["rulers"])))
        self.shell_var.set(self.parent.cfg["terminal_shell"])
        self.term_font_var.set(self.parent.cfg["terminal_font"])
        self.lang_var.set(self.parent.cfg["language"])
        self.lexer_var.set(self.parent.cfg["current_lexer"])

    def on_search(self, event=None):
        # Recherche simple : on filtre les labels des tabs
        query = self.search_entry.get().lower()
        for i in range(self.nb.index("end")):
            tab = self.nb.tab(i, "text").lower()
            if query in tab:
                self.nb.select(i)
                return


if __name__ == "__main__":
    # Create and run the main application
    app = Colony()
    try:
        app.mainloop()
    except KeyboardInterrupt:
        try:
            app.destroy()
        except Exception:
            pass
            