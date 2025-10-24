use anyhow::Result;
use indexmap::IndexMap;
use parking_lot::RwLock;
use std::collections::HashMap;
use std::sync::Arc;
use unic_langid::{LanguageIdentifier, langid};

#[derive(Debug, Clone)]
pub struct LocalizationProvider {
    active: RwLock<LanguageIdentifier>,
    bundles: Arc<IndexMap<LanguageIdentifier, HashMap<&'static str, &'static str>>>,
}

impl LocalizationProvider {
    pub fn new() -> Result<Self> {
        let bundles = build_bundles();
        let system_lang = sys_locale::get_locale()
            .and_then(|tag| tag.parse::<LanguageIdentifier>().ok())
            .unwrap_or_else(|| langid!("en-US"));
        let active = if bundles.contains_key(&system_lang) {
            system_lang
        } else {
            langid!("en-US")
        };
        Ok(Self {
            active: RwLock::new(active),
            bundles: Arc::new(bundles),
        })
    }

    pub fn set_locale(&self, lang: &LanguageIdentifier) {
        if self.bundles.contains_key(lang) {
            *self.active.write() = lang.clone();
        }
    }

    pub fn active_locale(&self) -> LanguageIdentifier {
        self.active.read().clone()
    }

    pub fn text(&self, key: &str) -> String {
        let locale = self.active_locale();
        if let Some(bundle) = self.bundles.get(&locale) {
            if let Some(value) = bundle.get(key) {
                return (*value).to_string();
            }
        }
        self.bundles
            .get(&langid!("en-US"))
            .and_then(|bundle| bundle.get(key))
            .map(|value| (*value).to_string())
            .unwrap_or_else(|| key.to_string())
    }

    pub fn available_locales(&self) -> Vec<LanguageIdentifier> {
        self.bundles.keys().cloned().collect()
    }
}

fn build_bundles() -> IndexMap<LanguageIdentifier, HashMap<&'static str, &'static str>> {
    let mut bundles = IndexMap::new();

    bundles.insert(
        langid!("en-US"),
        HashMap::from([
            ("menu_file", "File"),
            ("menu_edit", "Edit"),
            ("menu_view", "View"),
            ("menu_go", "Go"),
            ("menu_run", "Run"),
            ("menu_terminal", "Terminal"),
            ("menu_help", "Help"),
            ("menu_workspace", "Workspace"),
            ("menu_theme", "Theme"),
            ("menu_language", "Language"),
            ("menu_settings", "Settings"),
            ("menu_toggle_explorer", "Toggle Explorer"),
            ("status_ready", "Ready"),
            ("status_lsp_connecting", "Connecting to language server"),
            ("status_lsp_connected", "Language server connected"),
        ]),
    );

    bundles.insert(
        langid!("fr-FR"),
        HashMap::from([
            ("menu_file", "Fichier"),
            ("menu_edit", "Édition"),
            ("menu_view", "Affichage"),
            ("menu_go", "Aller"),
            ("menu_run", "Exécuter"),
            ("menu_terminal", "Terminal"),
            ("menu_help", "Aide"),
            ("menu_workspace", "Espace de travail"),
            ("menu_theme", "Thème"),
            ("menu_language", "Langue"),
            ("menu_settings", "Paramètres"),
            ("menu_toggle_explorer", "Basculer l'explorateur"),
            ("status_ready", "Prêt"),
            ("status_lsp_connecting", "Connexion au serveur de langage"),
            ("status_lsp_connected", "Serveur de langage connecté"),
        ]),
    );

    bundles.insert(
        langid!("es-ES"),
        HashMap::from([
            ("menu_file", "Archivo"),
            ("menu_edit", "Editar"),
            ("menu_view", "Ver"),
            ("menu_go", "Ir"),
            ("menu_run", "Ejecutar"),
            ("menu_terminal", "Terminal"),
            ("menu_help", "Ayuda"),
            ("menu_workspace", "Área de trabajo"),
            ("menu_theme", "Tema"),
            ("menu_language", "Idioma"),
            ("menu_settings", "Configuración"),
            ("menu_toggle_explorer", "Alternar explorador"),
            ("status_ready", "Listo"),
            (
                "status_lsp_connecting",
                "Conectando al servidor de lenguaje",
            ),
            ("status_lsp_connected", "Servidor de lenguaje conectado"),
        ]),
    );

    bundles.insert(
        langid!("de-DE"),
        HashMap::from([
            ("menu_file", "Datei"),
            ("menu_edit", "Bearbeiten"),
            ("menu_view", "Ansicht"),
            ("menu_go", "Gehe zu"),
            ("menu_run", "Ausführen"),
            ("menu_terminal", "Terminal"),
            ("menu_help", "Hilfe"),
            ("menu_workspace", "Arbeitsbereich"),
            ("menu_theme", "Design"),
            ("menu_language", "Sprache"),
            ("menu_settings", "Einstellungen"),
            ("menu_toggle_explorer", "Explorer umschalten"),
            ("status_ready", "Bereit"),
            ("status_lsp_connecting", "Verbindung zum Sprachserver"),
            ("status_lsp_connected", "Sprachserver verbunden"),
        ]),
    );

    bundles.insert(
        langid!("ja-JP"),
        HashMap::from([
            ("menu_file", "ファイル"),
            ("menu_edit", "編集"),
            ("menu_view", "表示"),
            ("menu_go", "移動"),
            ("menu_run", "実行"),
            ("menu_terminal", "ターミナル"),
            ("menu_help", "ヘルプ"),
            ("menu_workspace", "ワークスペース"),
            ("menu_theme", "テーマ"),
            ("menu_language", "言語"),
            ("menu_settings", "設定"),
            ("menu_toggle_explorer", "エクスプローラー切替"),
            ("status_ready", "準備完了"),
            ("status_lsp_connecting", "言語サーバーに接続中"),
            ("status_lsp_connected", "言語サーバー接続済み"),
        ]),
    );

    bundles.insert(
        langid!("zh-CN"),
        HashMap::from([
            ("menu_file", "文件"),
            ("menu_edit", "编辑"),
            ("menu_view", "视图"),
            ("menu_go", "转到"),
            ("menu_run", "运行"),
            ("menu_terminal", "终端"),
            ("menu_help", "帮助"),
            ("menu_workspace", "工作区"),
            ("menu_theme", "主题"),
            ("menu_language", "语言"),
            ("menu_settings", "设置"),
            ("menu_toggle_explorer", "切换资源管理器"),
            ("status_ready", "就绪"),
            ("status_lsp_connecting", "正在连接语言服务器"),
            ("status_lsp_connected", "语言服务器已连接"),
        ]),
    );

    bundles.insert(
        langid!("pt-BR"),
        HashMap::from([
            ("menu_file", "Arquivo"),
            ("menu_edit", "Editar"),
            ("menu_view", "Exibir"),
            ("menu_go", "Ir"),
            ("menu_run", "Executar"),
            ("menu_terminal", "Terminal"),
            ("menu_help", "Ajuda"),
            ("menu_workspace", "Área de trabalho"),
            ("menu_theme", "Tema"),
            ("menu_language", "Idioma"),
            ("menu_settings", "Configurações"),
            ("menu_toggle_explorer", "Alternar explorador"),
            ("status_ready", "Pronto"),
            (
                "status_lsp_connecting",
                "Conectando ao servidor de linguagem",
            ),
            ("status_lsp_connected", "Servidor de linguagem conectado"),
        ]),
    );

    bundles.insert(
        langid!("ru-RU"),
        HashMap::from([
            ("menu_file", "Файл"),
            ("menu_edit", "Правка"),
            ("menu_view", "Вид"),
            ("menu_go", "Переход"),
            ("menu_run", "Выполнить"),
            ("menu_terminal", "Терминал"),
            ("menu_help", "Справка"),
            ("menu_workspace", "Рабочее пространство"),
            ("menu_theme", "Тема"),
            ("menu_language", "Язык"),
            ("menu_settings", "Настройки"),
            ("menu_toggle_explorer", "Переключить проводник"),
            ("status_ready", "Готов"),
            ("status_lsp_connecting", "Подключение к языковому серверу"),
            ("status_lsp_connected", "Языковой сервер подключен"),
        ]),
    );

    bundles.insert(
        langid!("ar-SA"),
        HashMap::from([
            ("menu_file", "ملف"),
            ("menu_edit", "تحرير"),
            ("menu_view", "عرض"),
            ("menu_go", "انتقال"),
            ("menu_run", "تشغيل"),
            ("menu_terminal", "الطرفية"),
            ("menu_help", "مساعدة"),
            ("menu_workspace", "مساحة العمل"),
            ("menu_theme", "السمة"),
            ("menu_language", "اللغة"),
            ("menu_settings", "الإعدادات"),
            ("menu_toggle_explorer", "تبديل المستكشف"),
            ("status_ready", "جاهز"),
            ("status_lsp_connecting", "جارٍ الاتصال بخادم اللغة"),
            ("status_lsp_connected", "تم الاتصال بخادم اللغة"),
        ]),
    );

    bundles
}
