use std::process::{Child, Command, Stdio};
use std::sync::Mutex;
use std::time::{Duration, Instant};

use tauri::{Manager, RunEvent, WebviewUrl, WebviewWindowBuilder};

// 后端子进程句柄，退出时杀掉
struct Backend(Mutex<Option<Child>>);

// 定位侧车：Tauri 会把 externalBin 按目标三元组命名放在资源目录
fn backend_bin(app: &tauri::AppHandle) -> Option<std::path::PathBuf> {
    let name = format!("tour-backend{}", std::env::consts::EXE_SUFFIX);
    let mut candidates = Vec::new();
    if let Ok(dir) = app.path().resource_dir() {
        candidates.push(dir.join("binaries").join(&name));
        candidates.push(dir.join(&name));
    }
    if let Some(dir) = std::env::current_exe().ok()?.parent() {
        candidates.push(dir.join(&name));
    }
    candidates.into_iter().find(|p| p.is_file())
}

// 等后端 8080 端口就绪（最多约 10 秒）
fn wait_backend(timeout: Duration) -> bool {
    let t0 = Instant::now();
    while t0.elapsed() < timeout {
        if std::net::TcpStream::connect("127.0.0.1:8080").is_ok() {
            return true;
        }
        std::thread::sleep(Duration::from_millis(120));
    }
    false
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            // 先拉起内嵌后端，再打开主窗口指向 localhost:8080
            let child = backend_bin(&app.handle())
                .and_then(|bin| Command::new(bin).stdout(Stdio::null()).stderr(Stdio::null()).spawn().ok());

            if child.is_some() && wait_backend(Duration::from_secs(12)) {
                let win = WebviewWindowBuilder::new(
                    app,
                    "main",
                    WebviewUrl::External("http://localhost:8080".parse().unwrap()),
                )
                .title("景区旅游信息管理系统")
                .inner_size(1280.0, 800.0)
                .min_inner_size(960.0, 600.0)
                .build();
                if let Err(e) = win {
                    eprintln!("创建窗口失败: {e}");
                }
            } else {
                // 后端没起来：显示错误页
                let _ = WebviewWindowBuilder::new(
                    app,
                    "main",
                    WebviewUrl::App("error.html".into()),
                )
                .title("启动失败")
                .inner_size(640.0, 360.0)
                .build();
            }
            app.manage(Backend(Mutex::new(child)));
            Ok(())
        })
        .build(tauri::generate_context!())
        .expect("Tauri 应用初始化失败")
        .run(|app, event| {
            // 应用退出时杀掉后端子进程
            if let RunEvent::Exit = event {
                if let Some(state) = app.try_state::<Backend>() {
                    if let Some(mut child) = state.0.lock().unwrap().take() {
                        let _ = child.kill();
                    }
                }
            }
        });
}
