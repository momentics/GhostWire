# GhostWire Desktop

![Release](https://img.shields.io/github/v/release/momentics/GhostWire?label=version&color=blue)
![Build](https://img.shields.io/github/actions/workflow/status/momentics/GhostWire/build.yml?branch=main&label=build)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

| | |
|:---:|---|
| <img src="resources/icons/ghostwire.png" alt="GhostWire Logo" width="512"> | Графическая оболочка над нативной библиотекой **GhostWire** с защитой от DPI для **Telegram Desktop**. Приложение работает в системном трее и не отображает окон на рабочем столе. |

---

## Как это выглядит

При запуске в трее появляется анимированная иконка. Правый клик открывает компактное меню:

- **Статистика** — аптайм, количество активных соединений
- **Легенда RX/TX** — цветная маркировка входящего и исходящего трафика
- **График** — история трафика за последний час с autoscale по оси Y
- **Старт / Стоп** — переключатель приёма соединений
- **Выход** — завершение приложения

---
## Скриншоты

### Иконка в трее

<p align="center">
  <img src="resources/icons/TaskBar.png" alt="TaskBar icon" width="400">
</p>

### Контекстное меню

<p align="center">
  <img src="resources/icons/MenuBar.png" alt="Context menu" width="350">
</p>

---

## Управление

| Действие | Результат |
|---|---|
| **Правый клик** на иконку | Открыть контекстное меню |
| **Старт** | Запустить, начать приём соединений |
| **Стоп** | Остановить, очистить интерфейс |
| **Подключить Telegram** | Открыть диалог настройки прокси в Telegram Desktop |
| **Выход** | Завершить приложение |

---

## Поведение при запуске

- **Первый запуск** — GhostWire находится в состоянии "Стоп"
- **Последующие запуски** — приложение восстанавливает предыдущее состояние. Если при завершении работы прокси был запущен, он автоматически запустится снова. Если был остановлен — останется остановленным

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
