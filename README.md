# GhostWire Desktop

![Release](https://img.shields.io/github/v/release/momentics/GhostWire?label=version&color=blue) ![Platforms](https://img.shields.io/badge/platforms-Windows%207%2C8%2C8.1%2C10%2C11%20%7C%20Ubuntu%2022.04%20%7C%20macOS-lightgrey) ![License](https://img.shields.io/badge/license-MIT-green)

| | |
|:---:|---|
| <img src="resources/icons/ghostwire.png" alt="GhostWire Logo" width="512"> | Графическая оболочка над нативной библиотекой **GhostWire** с защитой от DPI (OSI L4-L7), для **Telegram Desktop**. Приложение работает в системном трее, в User Mode, не использует внешние сервисы и сервера. |

---

## Как это выглядит

При запуске в трее появляется анимированная иконка. Правый клик открывает компактное меню.

---
## Скриншоты

### Анимированная иконка в трее

<p align="center">
  <img src="resources/icons/TaskBar.png" alt="TaskBar icon" height="60"><img src="resources\icons\tray_idle.png" alt="IDLE TaskBar icon" width="16"><img src="resources\icons\tray_active.png" alt="ACTIVE TaskBar icon" width="16"><img src="resources\icons\tray_degraded.png" alt="DEGRADED TaskBar icon" width="16">
</p>


### Контекстное меню
Windows
<p align="center">
  <img src="resources/icons/MenuBar.png" alt="Context menu" width="350">
</p>
Linux
<p align="center">
  <img src="resources/icons/MenuBarUbuntu.png" alt="Context menu" width="350">
</p>
macOS
<p align="center">
  <img src="resources/icons/MenuBarMacOS.png" alt="Context menu" width="350">
</p>

---

## Управление

| Действие | Результат |
|---|---|
| **Правый клик** на иконку | Открыть контекстное меню |
| **Старт** | Запустить, начать приём соединений |
| **Стоп** | Остановить, очистить интерфейс |
| **Подключить Telegram** | Открыть диалог настройки прокси в Telegram Desktop |
| **Проверить обновления** | Проверить наличие новой версии на GitHub |
| **Выход** | Завершить приложение |

---

## Поведение при запуске

- **Первый запуск** — GhostWire находится в состоянии "Стоп"
- **Последующие запуски** — приложение восстанавливает предыдущее состояние. Если при завершении работы прокси был запущен, он автоматически запустится снова. Если был остановлен, то останется остановленным.

---

## Использование с Telegram

После нажатия **Старт**, можно выбрать **Подключить Telegram**:


<p align="center">
  <img src="resources/icons/TGBar.png" alt="Telergram Proxy SetUP" width="350">
</p>

Или установить конфигурацию самостоятельно:

1. Telegram Desktop → Настройки → Продвинутые → Тип соединения → SOCKS5
2. Сервер: `127.0.0.1`, Порт: `1080`
3. Сохранить

---

## Проверка обновлений

При запуске приложение автоматически проверяет наличие новой версии на GitHub (не чаще одного раза в 24 часа). При обнаружении обновления появляется уведомление в трее со ссылкой на страницу релиза.

Ручная проверка: кнопка **Проверить обновления** в контекстном меню.

---

## Поддержать автора

"Голодающим поволжья"  :pray:  **OZON Bank: 2204 2402 8673 4225**

---

