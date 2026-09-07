# AGENTS.md

C++23 library for calendar/personal activities (domain model). Namespace `events` (lowercase). The whole logical model lives in `model/`; the repo root holds `docker-compose.yml` (builds the model service), README, LICENSE, this file, plus the standalone `app/` and the delivery artifacts (`Attivita.pro`, `examples/`, `relazione/`). **`server/` and `db/` (REST API + PostgreSQL) were removed from the repo entirely** (commit "rimosso stack db/server e servizi docker-compose collegati") — the parts of this file below that still describe them are kept only as historical/aspirational spec, not as current layout.

> **Git workflow:** ogni volta che vengono fatte modifiche al codice, va creato un commit (branch corrente: `refactor`). Commit atomic, messaggi concisi in italiano, coerenti con lo stile del log esistente. Non fare push/PR a meno che non sia esplicitamente richiesto.

> **Delivery target (PAO course spec 2025/26):** the app to be delivered must compile in the professor's container (`unipd-oop/qt-env:2025`, Ubuntu 24.04 + g++ 13.3 + cmake 3.28 + **Qt 6.4.2 + qmake 3.1**, NO Catch2/nlohmann/libpqxx) and run **standalone** with local JSON file persistence. The REST server + PostgreSQL stack is an optional extra, off the evaluation critical path. Roadmap: Fase 0 (env check) + Fase 1 (Activity hierarchy) + Fase 2 (JSON persistence via Visitor) + Fase 3 (standalone Qt single-window app) + Fase 4 (delivery: `Attivita.pro` qmake, `examples/attivita_esempio.json`, `pack_delivery.sh` zip script, verified build-from-zip in the prof container) + Fase 5 (PDF report: `relazione/relazione.tex`, compiles 4pp) DONE. Remaining for the student: UML figure (`relazione/figura/`), name/matricola, actual hours in the report table.

## Stato del modello (branch `snello`, derivato da `refactor`)

> Il modello (`model/`), la persistenza JSON (`model/persistence/`) e l'applicazione Qt (`app/`) sono rifattorizzati e **compilano/passano i test con successo**. L'architettura dei generatori è **stateless e immutabile**; la deduplicazione (Flyweight) e il limite di occorrenze descritti in versioni precedenti di questo file **non sono implementati nel codice attuale** — vedi sotto.

- [x] **`Activity` concreta (non astratta) & derivati (`Task`, `Meeting`)**:
  - `Activity` **non è astratta**: è la classe concreta "evento singolo/ricorrente" e possiede `m_title`, `m_start`, `m_end` (default: `TimePoint::max()`), `m_duration`, le eccezioni (`m_exceptions`) e un puntatore condiviso immutabile al generatore (`std::shared_ptr<const DateGenerator>`). **Non esiste `m_maxOccurrences`**: il limite di occorrenze non è (più) una feature del modello.
  - `title` e `start` sono **obbligatori** nei costruttori di `Activity`, `Task` e `Meeting`.
  - `Task` e `Meeting` sono le uniche due sottoclassi di `Activity` (**3 tipi concreti in totale**, non 5: non esistono `Event`, `RecurrentEvent` né `Anniversary`, rimossi durante il refactor). `Task` gestisce il completamento per-occorrenza (`m_doneOccurrences`).
  - `Activity` supporta copia e assegnamento (Value Semantics, generatore condiviso via `shared_ptr`) e clonazione tramite `clone()`.
- [x] **DateGenerator STATELESS & IMMUTABILI**:
  - `DateGenerator` è una pura regola di calcolo matematico/calendariale (senza `m_start` né `m_end`).
  - Interfaccia: `next(TimePoint current)`, `align(TimePoint start, TimePoint from)` (O(1)) e `occurrences(TimePoint start, TimePoint from, TimePoint limit) -> std::generator<TimePoint>` — un **coroutine generator C++23** (header `<generator>`, non un `view_interface` custom) che produce pigramente la sequenza di date.
  - Implementa l'interfaccia **`utils::Cacheable`** (`isEqualImpl` + `hash`, usata da `operator==` con confronto per `typeid`) e il Visitor pattern (`accept(DateGeneratorVisitor&)`).
  - Classi concrete: `SingleGenerator()` (stateless, `next` restituisce `TimePoint::max()`), `FixedIntervalGenerator(interval)`, `MonthlyGenerator(months = 1)` (leap/end-of-month clamping aware), `YearlyGenerator(years = 1)` (29/2→28/2 fallback).
- [ ] **NON implementato: deduplicazione via Flyweight (`GeneratorPool`)**: nessun file `GeneratorPool`/`WeakCacheableHash`/`WeakCacheableEqual` esiste in `model/`. `utils::Cacheable.h` fornisce solo i functor non-weak `CacheableHash`/`CacheableEqual`, non ancora usati per una pool condivisa.
- [x] **Costruzione via Config + factory (non Builder)**:
  - Le classi `ActivityBuilder`/`TaskBuilder`/`MeetingBuilder`/`GeneratorBuilder` descritte in versioni precedenti **non esistono**: sostituite da struct aggregate in `builders/ActivityConfig.h` (`ActivityConfig`, `TaskConfig : ActivityConfig`, `MeetingConfig : ActivityConfig`) e da funzioni libere `makeActivity(ActivityConfig)`, `makeTask(TaskConfig)`, `makeMeeting(MeetingConfig)` che restituiscono l'`unique_ptr` corrispondente. Fallback generatore: `nullptr` → `SingleGenerator` condiviso statico (gestito nel costruttore di `Activity`). Non esistono `withMaxOccurrences`/`limitTo`/`MaxOccurrencesDecorator`.
- [x] **Persistenza JSON (`JsonPersistence`)** — aggiornata e funzionante:
  - Discriminatore `"type"` con **soli 3 valori**: `event | task | meeting` (non `recurrent`/`anniversary`, che non esistono più).
  - I generatori serializzano solo `type` (`single|fixed|monthly|yearly`) + la propria frequenza (`interval_seconds`/`interval_months`/`interval_years`); `title`, `start`, `duration_seconds`, `end` (opzionale), `exceptions` sono a livello di `Activity`. **Non esiste `max_occurrences`** nel formato JSON.
  - Deserializzazione tramite `std::shared_ptr<const DateGenerator>`.
- [x] **Pipeline C++23 Ranges per le occorrenze** — già implementata in `Activity::occurrencesIn`: compone `m_generator->occurrences(...)` (coroutine `std::generator`) con `std::views::filter` (eccezioni) + `std::views::transform` (→ `Occurrence`) + `std::ranges::to<std::vector>()`, in un unico passaggio lazy. Non usa una classe `OccurrenceRange`/`view_interface` dedicata.
- `server/`, `db/` — **rimossi dal repository** (non solo "da aggiornare"): la directory non esiste più su questo branch. `app/` compila (viste e form aggiornati alle Config; `ActivityFactory`, `MoveGeneratorVisitor` e `truncateBefore` non esistono più).

## Build & test

CMake is the primary build. Run from `model/` (needs Qt6 — or Qt5 — with the Core and Test components; tests are QTest-based, registered as `EventsTest`/`PersistenceTest`):

```bash
cmake -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

Containerized (docker compose): compiles the tests, runs them and generates the Doxygen docs in `model/docs/`:

```bash
docker compose up --build model
```

## Layout

- `model/` — the entire logical model (the `events` library + its tests):
  - `model/include/{core,domain,generators,builders,utils}/` — public headers, code includes them as `"core/Activity.h"` etc. (require `-Iinclude`); entry point `model/include/events.h` (all-in-one, `#include "events.h"`). No extra `events/` folder level: the namespace is `events`, but nothing in the path repeats it (removed as redundant — the include root already IS `model/include`).
  - `model/src/{core,domain,generators}/` — implementation, mirrors `include/` 1:1 (no `builders/`/`utils/` under `src/`: `ActivityConfig.h`/`Cacheable.h` are header-only). Picked up by `file(GLOB_RECURSE ... src/*.cpp)` in `model/CMakeLists.txt`, recursive so new subfolders need no CMake change.
  - All headers use `#pragma once` (no `#ifndef`/`#define` guards) — same convention repo-wide (`model/` and `app/`).
  - `model/tests/test.cpp` + `model/tests/test_persistence.cpp` — the test files. **QTest** (`QObject`-derived `TestModel`, `private slots:`, `QCOMPARE`/`QVERIFY`), NOT Catch2 — model tests no longer use Catch2 at all.
  - `model/persistence/` — JSON persistence module, library `events_persistence` (Qt Core ONLY, no Widgets): `persistence::activityToJson/calendarToJson/activityFromJson/calendarFromJsonArray/saveToFile/loadFromFile`. Serialization uses the Visitor pattern (`JsonActivityVisitor` + `JsonGeneratorVisitor`); deserialization is a discriminator-based factory ("type" field `event|task|meeting`, validated strictly — only 3 activity types exist). File format: `{"version":1, "activities":[...]}`, times as ISO-8601 UTC `YYYY-MM-DDTHH:MM:SS`. Solo il **Task** serializza il flag `done` (per-occorrenza, `done_occurrences`); Meeting `location`+`attendees`, Task `priority`. Built only when Qt6 Core is found.
  - `model/Doxyfile` + `model/docs/` — Doxygen config (docs/ is gitignored).
  - `model/Dockerfile` + `model/.dockerignore` — builds the tests and generates the docs. Installs `qt6-base-dev` (needed for both QTest and the persistence module). It also still apt-installs `catch2`, a **stale leftover** from before the switch to QTest — no CMake target references Catch2 in `model/` anymore.
  - CMake (`model/CMakeLists.txt`, `CMAKE_CXX_STANDARD 23`): `find_package(Qt6 COMPONENTS Core Test QUIET)` (Qt5 fallback) — tests (`events_tests`, `persistence_tests`) are built only when the **Qt Test** component is found; `TARGET Qt::Core OR TARGET Qt5::Core` gates the persistence module. No Catch2 dependency anywhere in `model/`.
- `app/` — Qt6 desktop app **standalone** (single-window, deliverable PAO; native build, NOT containerized; MVC: `CalendarController` + menu + views):
  - `app/src/` e' diviso per responsabilita' (come `model/`, header separati dall'implementazione): `app/src/controller/`, `app/src/menu/`, `app/src/views/` per i `.cpp`, e `app/include/{controller,menu,views}/` per i rispettivi header — `#include "views/X.h"`/`"menu/X.h"`/`"controller/X.h"` risolvono via quell'`include/` (`target_include_directories(pao_core PUBLIC src include)`). **Nota CMake/AUTOMOC**: con header e `.cpp` in cartelle diverse, AUTOMOC non li accoppia piu' per nome/stessa-cartella — ogni header con `Q_OBJECT` va quindi elencato esplicitamente anche lui tra i sorgenti di `add_library`/`add_executable` (oltre al proprio `.cpp`), altrimenti il link fallisce con `undefined reference to vtable for ...`/`staticMetaObject`.
  - `app/src/controller/CalendarController.cpp` (+ header in `app/include/controller/`) — controller MVC (QObject): possiede `events::Calendar`, CRUD + `search` + `occurrencesIn`, `addActivities` (piu' serie in un colpo), azioni sulle occorrenze (`deleteOccurrence` → EXDATE o truncate "questa e le successive", `modifyOccurrence` → eccezione interna + evento singolo, `updateActivity` conserva le eccezioni), **`toggleDone(Occurrence)`** (inverte lo stato evaso/da fare solo di un COMPITO, l'unico tipo con stato), persistenza via `events_persistence`. Segnale `activitiesChanged` → le viste si aggiornano.
  - `app/src/menu/AppMenuBar.cpp` (+ header in `app/include/menu/`, incluso `MenuShortcutStyle.h`) — vedi `AppMenuBar` sotto.
  - `app/src/views/` (+ header in `app/include/views/`) — viste Qt (MVC, mai path cablati); sotto-diviso ulteriormente in due sottocartelle (sia in `src/views/` che in `include/views/`, stesso schema di `controller/menu/views`):
    - `views/utils/` — helper condivisi senza logica applicativa propria (`ViewShared.h`, `WidgetUtils.h`, `Theme.h`, vedi sotto): solo header, nessun `.cpp` (`#include "views/utils/X.h"`).
    - `views/dialog/` — i pannelli di creazione/modifica/dettaglio di un'attivita' e i loro helper di visualizzazione: `ActivityFormDialog`, `ActivityFormPage`, `ActivityDetailDialog`, `RecurrenceChoiceDialog`, `ActivityViewHelpers` (`.h` in `include/views/dialog/`, `.cpp` in `src/views/dialog/`, `#include "views/dialog/X.h"`).
    - Il resto (`MainWindow`, `WeekView`/`DayView`/`MonthView`/`YearView`, `ActivityListPage`, `OccurrenceWidget`, e i widget di composizione di `WeekView` — `HeaderWidget`, `AllDayAreaWidget`, `TimeGutterWidget`, `DayColumnWidget`, vedi sotto) resta direttamente in `views/` (non e' ne' un dialogo ne' un helper condiviso).
    - `MainWindow` — finestra unica (vincolo PAO) con QStackedWidget a **5 pagine**: 0=settimana, 1=elenco, 2=giorno, 3=mese, 4=anno. La barra dei menu e' un widget separato (`AppMenuBar : public QMenuBar`, vedi sotto): la MainWindow si limita a `setMenuBar(m_menuBar)` e a collegarne i segnali (`saveRequested`/`saveAsRequested`/`loadRequested`/`viewSelected`/`newActivityRequested`) ai propri slot (`onSave`/`onSaveAs`/`onLoad`/`onViewSelected`/`openNewActivityType`), niente logica di costruzione dei menu al suo interno. **Barra di navigazione condivisa** — widget a se stante (`NavigationBar`, in `views/`, non un dialog ne' un util: e' una vista come `WeekView`/`ActivityListPage`) con i pulsanti Oggi/<-/-> e l'etichetta del periodo; non sa nulla di date o della vista attiva, emette solo `todayRequested`/`previousRequested`/`nextRequested` e la MainWindow imposta il testo con `setLabel(text)`. Estratta fuori dallo stack (nascosta su Elenco). La MainWindow traduce le richieste spostando la `QDate` di riferimento `m_anchor` (giorno:+-1, settimana:+-7, mese:+-1 mese, anno:+-1 anno; `setAnchor` la normalizza per la vista corrente: lunedi' / 1 del mese / 1 gennaio) — il passo dipende dalla vista attiva, quindi questa logica resta nella MainWindow e non nel widget. Account per vista: `ViewKind{Day,Week,Month,Year}` (privato, usato per la navigazione — distinto da `AppMenuBar::ViewKind`, che include anche `List`). Dettaglio non ha spunta nel menu. Pannelli interni `ActivityDetailDialog`, `ActivityFormDialog` + `RecurrenceChoiceDialog` (widget figli, ricentrati in `resizeEvent`, in `views/dialog/`).
    - `AppMenuBar` — costruisce da sola tutti i menu: **File** (Salva calendario **Ctrl+S** — salva sull'ultimo file usato, altrimenti chiede il nome / Salva con nome **Ctrl+Shift+S** / Carica calendario **Ctrl+O**, via QFileDialog; le scorciatoie sono disegnate a DESTRA della voce (posizione standard) piu' attenuate del testo tramite lo stile `MenuShortcutStyle`, colori presi dalla QPalette corrente — non fissi — cosi' seguono anche il tema scuro), **Visualizza** (Elenco/Giorno/Settimana/Mese/Anno, azione attiva spuntata con QActionGroup, sincronizzata dall'esterno con `setActiveView(ViewKind)`) e **Nuova attivita'** (Evento/Riunione/Compito/Anniversario, piu' la scorciatoia **Ctrl+N** per il tipo Evento) → segnale `newActivityRequested(int)`.
    - `ViewShared.h` (in `views/utils/`) — helper condivisi inline, solo Qt Gui (niente Widgets: incluso anche da `app_controller_tests`, che collega Qt Test + Qt Gui ma non Qt Widgets): `activityColor` (paletta stabile dall'indirizzo dell'oggetto), `localTime` (UTC->locale), `shortDayName(dayOfWeek)`.
    - `WidgetUtils.h` (in `views/utils/`) — controparte di `ViewShared.h` per l'unico helper che richiede Qt Widgets: `repolish(QWidget*)` (rivaluta le regole QSS dipendenti da una proprieta' dinamica dopo un `setProperty`, via `style()->unpolish/polish`); in un header separato apposta per non tirare Widgets dentro `ViewShared.h`.
    - `Theme.h` (in `views/utils/`) — colori condivisi per il codice che un foglio di stile Qt non puo' raggiungere (disegno `QPainter` diretto in `WeekView::paintEvent`, colori calcolati a runtime in `OccurrenceWidget`/`YearView`): gli stessi valori esadecimali sono duplicati in `resources/style.qss` per il resto dell'interfaccia, che invece Qt sa colorare da un file di stile — tenerli allineati a mano se si cambia un colore.
    - `WeekView` — griglia settimanale composta da widget Qt reali (non piu' un unico canvas `QPainter`), **numero di giorni configurabile** (`setDayCount`, default 7; usato da `DayView` con 1):
      ```
      WeekView (QVBoxLayout)
      +-- HeaderWidget          intestazione giorni, FISSA in alto
      +-- AllDayAreaWidget      striscia "tutto il giorno", FISSA
      +-- QScrollArea           SOLO verticale (orizzontale sempre off, verticale
           +-- QWidget               sempre on cosi' la larghezza riservata e' costante
                +-- QHBoxLayout      e header/striscia possono allinearsi alle colonne)
                     +-- TimeGutterWidget      colonna ore 00:00..23:00 (24 QLabel fissi)
                     +-- DayColumnWidget x N   una per giorno, elastica in larghezza
      ```
      `WeekView` stessa non ha piu' ne' `paintEvent` ne' gestione di mouse/drag&drop: e' solo composizione/smistamento. Ogni `DayColumnWidget` e' **autonoma per interazione** (clic su cella vuota, doppio clic, menu contestuale, drag&drop sorgente E destinazione, anteprima live) e possiede i propri `OccurrenceWidget` (posizionati assolutamente al proprio interno, coordinate LOCALI = minuti dalla mezzanotte, altezza fissa `24*kWeekHourHeight`: scorre nella QScrollArea, non scala piu' col ridimensionamento). `AllDayAreaWidget` usa un `QGridLayout` con **column-span** (una colonna per giorno + una riservata al gutter) per le occorrenze che coprono uno o piu' giorni interi (`coversFullDay`), impilate su righe da `WeekGridLayout::layoutAllDayRows`; i suoi chip non sono trascinabili (come prima). La **selezione** (clic sinistro) e' esclusiva su tutta la griglia a prescindere da quale colonna possiede il chip, con lo stesso schema di `MonthView`/`MonthDayCell`: `WeekView` tiene solo `OccurrenceWidget* m_selectedChip` + `optional<Occurrence> m_selectedOccurrence`, aggiornati da un segnale `chipPressed(chip, occurrence)` che ogni colonna/l'area all-day emette bubbling verso l'alto. **Spunta (checkbox) solo sui Compiti** → `doneToggled` → `CalendarController::toggleDone`; **anteprima live** `Preview{title,start,duration}` inoltrata a TUTTE le colonne (ognuna si auto-filtra confrontando la propria data); **drag&drop** gestito per-colonna (ogni `DayColumnWidget` e' sorgente/destinazione nativa Qt, con un proprio `QRubberBand`): rilascio su una nuova cella emette `activityMoved` -> `CalendarController::moveActivity` (stessa semantica di prima su ricorrenti/eccezioni). L'interfaccia pubblica (9 segnali, `setOccurrences`/`setWeekStart`/`setPreview`/`selectedOccurrence`/`setDayCount`) e' INVARIATA rispetto a prima del refactor: `MainWindow` non ha bisogno di sapere che la griglia e' ora composta da widget diversi.
      - `HeaderWidget`/`AllDayAreaWidget`/`TimeGutterWidget`/`DayColumnWidget` vivono in `views/` (non `views/utils/`: hanno stato/segnali propri, non sono helper puri).
      - `WeekGridLayout` (in `views/utils/`, invariato come posizione) resta l'unico posto con calcolo puro (nessun `QWidget`/`QPainter`, testabile senza Widgets), ma non piu' un unico `place()` per l'intera griglia: due funzioni piu' piccole, ciascuna usata da un solo widget.
        - `layoutAllDayRows(occurrences, viewStart, dayCount) -> vector<AllDayItem{index,firstDay,lastDay,row}>` — impila le "tutto il giorno" su righe (greedy), usata da `AllDayAreaWidget` per gli `addWidget(chip, row, 1+firstDay, 1, lastDay-firstDay+1)` del suo `QGridLayout`.
        - `layoutDayColumn(dayOccurrences, columnWidth) -> vector<QRect>` — affianca in colonne le occorrenze sovrapposte di UN SOLO giorno (interval-graph greedy coloring), in coordinate LOCALI a quella colonna; usata da `DayColumnWidget::relayout()` (richiamata anche a ogni `resizeEvent`, dato che la larghezza e' elastica).
        - Le costanti geometriche (`kWeekGutterWidth`/`kWeekHeaderHeight`/`kWeekAllDayRowHeight`/`kWeekHourHeight`/`kWeekMinOccurrenceHeight`/`kWeekDaysPerWeek`) sono ora FISSE (non piu' scalate a runtime col ridimensionamento: quel ruolo lo svolge la QScrollArea), condivise da tutti i widget della griglia.
      - `WeekGridPainter` **non esiste piu'**: ogni sua responsabilita' e' ora un widget reale (intestazione → `HeaderWidget`, sfondo striscia → `AllDayAreaWidget`, ore → `TimeGutterWidget`) tranne le sole linee orarie orizzontali, disegnate da un piccolo `paintEvent` locale in `DayColumnWidget` (unico `QPainter` residuo, il bordo sinistro incluso).
    - `DayView` — sottoclasse di `WeekView` con `setDayCount(1)` (invariata: nessuna modifica di questo refactor).
    - `MonthView` — griglia mensile 7x5-6 QPainter: numero del giorno (oggi evidenziato, fuori-mese in grigio) e **chip colorati** (max 3/giorno + "+N"); ogni chip dei **Compiti** ha la **spunta** → `doneToggled`; clic sinistro seleziona, doppio clic modifica (con scelta serie/istanza per i ricorrenti, come in WeekView), doppio clic su cella vuota crea alle 09:00, menu contestuale, tooltip; niente drag&drop ne' anteprima live.
    - `YearView` — 12 mini-calendari in griglia 4x3 (un mese ciascuno, nomi giorni + numeri) con pallini colorati per le attivita' (grigi se Compito evaso); doppio clic su un giorno -> `daySelected` -> la MainWindow passa alla vista giorno di quella data; tooltip con i titoli del giorno.
    - `ActivityListPage` (tabella a 4 colonne: **pallino del colore dell'attivita'** (come le griglie del calendario) + Titolo/Tipo/**Dettagli**; ricerca live per titolo via `Calendar::search` + **filtro per tipo** (QComboBox "Tutti i tipi"/Evento/Ricorrente/Riunione/Compito/Anniversario) + **checkbox "Solo da fare"** (nasconde i Compiti evasi); ordinamento a clic sull'intestazione: su "Titolo" ordina per titolo con "Tipo" come chiave secondaria (e viceversa su "Tipo"; default per data di inizio), indicatore di sort mantenuto tra i refresh; stile pulito: fondo bianco, righe alternate, header a contrasto, **separatori verticali grigi tra le colonne**, **testo nero 13pt e righe alte per leggibilita'**). `ActivityDetailDialog` (pannello ridotto DENTRO la MainWindow — widget figlio, non esce dai bordi: boundedTo(host), ricentrato in `MainWindow::resizeEvent`; titolo 18pt centrato rispetto alla finestra (margine sinistro pari alla larghezza della "X") e righe "campo: valore" 13pt centrate e distanziate come paragrafi; "X" (fuori dalla riga del titolo, in alto a destra dell'angolo) per chiudere + pulsanti in basso "Modifica" -> `editRequested` (apre il form) e "Elimina").
    - `ActivityFormPage` (form per tipo in QStackedWidget: **Evento "a domande"**/Riunione/Compito/Anniversario; modi Create/EditActivity/EditOccurrence). Il pannello **Evento NON chiede il tipo**: chiede titolo, data (slot separati), ora, durata e le checkbox **"Si ripete?"** e **"Tutto il giorno"**. Se si ripete: unita' (giorni/settimane/mesi/anno), "ogni N", per le **settimane** i **giorni della settimana** (pulsanti cliccabili Lun..Dom, si colorano) e la **fine** (radio: Mai / Fino al [data] / Dopo N occorrenze). La checkbox **"Tutto il giorno"** nasconde ora e durata e istanzia un `Event` dalle 00:00 di 24h (o, con "Si ripete", una serie che ricorre a giornate intere): la GUI mostra queste occorrenze nella striscia in alto. In base alle risposte istanzia un `Event` oppure uno o piu' `RecurrentEvent` (**uno per giorno della settimana** selezionato, "ogni lun e ven" = 2 serie settimanali). "Ogni N mesi" usa `MonthlyGenerator`; "fino a" imposta `end`, "dopo N" `GeneratorBuilder::limitTo`. Il pannello Riunione ha Luogo + elenco Partecipanti (Aggiungi con Invio / Rimuovi con doppio clic); campi propri per pannello in QGridLayout etichetta a sinistra/box affiancata, titolo e data conservati al cambio tipo via `syncCommonFields` — niente widget condivisi tra i pannelli. `CalendarController::addActivities` aggiunge piu' serie in un colpo solo.
  - `app/tests/test_controller.cpp` — test **QTest** del controller (headless, in ctest): `QObject`-derivata `TestController`, `private slots:`, `QCOMPARE`/`QVERIFY`, coerente con `model/tests/`. Un `init()` ricrea un `CalendarController` (`std::unique_ptr`) prima di ogni slot, per una fixture pulita ad ogni test (equivalente alle `SECTION` Catch2 di prima, ora slot separati). Registrato in ctest come `AppControllerTest` (un solo esito per l'intero binario, come `EventsTest`/`PersistenceTest` in `model/` — non piu' un esito per scenario come con `catch_discover_tests`).
  - `app/resources/style.qss` (+ `app/resources/app.qrc`) — foglio di stile Qt unico per l'intera GUI, incorporato come risorsa Qt e caricato una sola volta in `main.cpp` (`qApp->setStyleSheet(...)`); i widget non costruiscono piu' CSS a runtime, impostano solo `objectName`/proprieta' dinamiche/`QPalette` (vedi `WidgetUtils::repolish` e `Theme.h`).
  - `app/CMakeLists.txt` — Qt6 Widgets+Core, AUTOMOC + AUTORCC (per `resources/app.qrc`); `add_subdirectory(../model)`; target `pao_core` (controller, testabile senza Widgets) + `pao_app` + `app_controller_tests` (solo se trovato il modulo **Qt Test**, come in `model/`).
  - Build: `cmake -B build && cmake --build build -j && ctest --test-dir build` da `app/`. Compila nel container del professore (Qt 6.4.2): attenzione a NON usare API Qt ≥ 6.5 (es. `QTimeZone::UTC` non esiste lì — usare `QTimeZone(0)`); `QDateTime` UTC via `QTimeZone(0)` o `Qt::UTC` (deprecato solo su Qt recenti).
- `docker-compose.yml` (repo root) — **single service** `model` (test+docs pipeline, builds `model/Dockerfile`, mounts `./model:/app`). The previous `db`/`api` services were removed along with `db/` and `server/`.
- **Delivery artifacts** (repo root): `Attivita.pro` (qmake single-target app build, verified with `qmake6 && make` in the prof container), `examples/attivita_esempio.json` + `examples/ex.json` (sample persistence files for `events_persistence`), `relazione/relazione.tex` (PAO report, 10pt ≤8pp, compiled with pdflatex; UML placeholder in `relazione/figura/uml_attivita.png` — to be drawn by the student; student name/matricola via `\studente`/`\matricola`). `pack_delivery.sh` is documented as the Moodle-zip script but **was not found anywhere in the repository or its history** — verify it still exists (it may be untracked, `.gitignore` excludes `*.sh`) before relying on this description.

> The "DB semantics" and REST "API" endpoint table that used to live here described `server/`+`db/`, both **removed from the repo**. See the "Summary project intentions" section at the bottom of this file for that spec, kept only as historical/aspirational reference.

## Gotchas

- `DateGenerator::occurrences` / `Activity::occurrencesIn` ranges are INCLUSIVE on both ends. To get N weekly occurrences from a recurrence starting at `start`, query `[start, start + (N-1)*1_weeks]`.
- **All tests (model AND app) use QTest, not Catch2**: `model/tests/test.cpp`, `test_persistence.cpp` and `app/tests/test_controller.cpp` are all `QObject`-derived classes with `private slots:` and `QCOMPARE`/`QVERIFY`, gated by `find_package(Qt6 COMPONENTS ... Test)`. There is no Catch2 anywhere in the repository — do not add `Catch2::Catch2WithMain`/`catch_discover_tests` back.
- **`<generator>`/`std::generator` risk on the delivery container**: `DateGenerator::occurrences` is a C++23 coroutine generator (`model/CMakeLists.txt` sets `CMAKE_CXX_STANDARD 23`). libstdc++ only gained `<generator>` support starting with **GCC 14**; the professor's container ships **g++ 13.3**. Verify `#include <generator>` actually compiles there before delivery — if it doesn't, `Activity::occurrencesIn`/`DateGenerator::occurrences` need a non-coroutine fallback.
- Calendar literals like `2026y/2/28` compile under `-std=c++23` without extra flags; `_weeks`/`_years` literals live in `namespace events` (`CommonTypes.h`). There is NO `_days` literal — use `Days(n)`.
- Library is header+source with no Qt dependency (Qt is only used by the optional `events_persistence` module and its tests); `model/Dockerfile` installs build tools + `qt6-base-dev` + doxygen to compile/test `model/` and generate `model/docs/`. It also apt-installs `catch2`, which is now unused dead weight (tests moved to QTest).
- **Professor's delivery container** (`qt-env-prof:2025`, reproduced from `unipd-oop/qt-env:2025` Dockerfile): Ubuntu 24.04, g++ 13.3, cmake 3.28.3, Qt 6.4.2 + qmake 3.1 (qt6-base/svg/charts/multimedia/declarative), libsqlite3; NO Catch2, NO nlohmann-json, NO libpqxx. The `events` library MUST compile there. GUI code must stay Qt ≤ 6.4 compatible. `server/`/`db/` no longer exist in this repo (see top of file).


# Current Model Logic (implemented)

Namespace `events`. Abstractions in `model/include/core/`, concrete classes in `model/include/domain/` and `model/include/generators/`, implementations 1:1 in `model/src/{core,domain,generators}/`. Entry point: `events.h` (relative to `model/include/`).

## Class hierarchy

```
Activity (CONCRETE, not abstract)           // root + "single/recurring event" type: title + start + duration + generator
├── Task                                    // Compito: due (== start) + Priority{Low,Medium,High} + done per-occorrenza
└── Meeting                                 // Riunione: interval + location + attendees
DateGenerator (abstract)                    // stateless recurrence rule (Strategy)
├── SingleGenerator                         // no recurrence (next() -> TimePoint::max())
├── FixedIntervalGenerator                  // fixed Duration interval
├── MonthlyGenerator                        // calendar-month steps (leap/end-of-month clamping)
└── YearlyGenerator                         // calendar-year steps, leap-year aware
Calendar                                    // OWNS the polymorphic activities (vector<unique_ptr<Activity>>)
```

**Only 3 concrete activity types exist** (`Activity`, `Task`, `Meeting`) — there is **no** `Event`, `RecurrentEvent` or `Anniversary` class; those were removed in the refactor and any prior documentation mentioning them is stale. `Activity` itself is instantiated directly to represent both single and recurring events (recurrence comes entirely from the attached `DateGenerator`, not from a distinct subclass). Solo **Task** ha uno stato di completamento ("evaso/da fare", `isDone()/setDone()`, per-occorrenza via `m_doneOccurrences`); gli altri tipi non hanno stato. `Activity::addException/clearExceptions`, `Task::isOverdue/priorityLabel`, `Meeting::addAttendee/removeAttendee`.

**Non esiste un limite di occorrenze** ("Dopo N occorrenze") nel modello attuale: nessun `MaxOccurrencesDecorator`, nessun campo `max_occurrences` (né in `Activity` né nel JSON). Solo `m_end` (`TimePoint::max()` di default) limita una serie nel tempo. `MonthlyGenerator` fa passi di mesi CALENDARIALI (il 31 clampa all'ultimo giorno del mese, 31/1→28/2).

> **Nota (all-day):** NON esiste un tipo `AllDayEvent`. Un evento "tutto il giorno" e' una normale `Activity` (o occorrenza di serie) che parte alle 00:00 e dura 24h (fino alle 00:00 del giorno dopo). La GUI decide se mostrarlo nella **striscia in alto** in base a date e durate: `coversFullDay(occ)` (ViewShared.h) e' true se l'occorrenza copre almeno un giorno di calendario intero.

### `Activity` (core/Activity.h)
Classe **concreta** (non astratta): `m_title`, `m_start`, `m_end` (default `TimePoint::max()`), `m_duration`, `m_exceptions`, `std::shared_ptr<const DateGenerator> m_generator` (default `SingleGenerator` statico condiviso se non fornito nel costruttore). `title`/`start` obbligatori nel costruttore; supporta copia/assegnamento value-semantics. Interfaccia non banale:
- `occurrencesIn(from, to) -> vector<Occurrence>` — espande `m_generator->occurrences(m_start, from, min(m_end, to))` (coroutine `std::generator<TimePoint>`), filtra le eccezioni con `std::views::filter` e mappa a `Occurrence` con `std::views::transform`, materializzando con `std::ranges::to<std::vector>()`. `Task`/`Meeting` ereditano l'implementazione senza override. **Range INCLUSIVE on both ends**.
- `accept(ActivityVisitor&)` — double dispatch (Visitor pattern, usato da `JsonPersistence`).
- `describe() -> String` — per-type human summary (display only), override in `Task`/`Meeting`.
- `clone() -> unique_ptr<Activity>` — virtual (`std::make_unique<Derived>(*this)`), override in `Task`/`Meeting`.

### `Occurrence` (core/Occurrence.h)
Value type: `{const Activity* source (non-owning), start, duration}`, `end()`, `overlaps(from,to)`. Lightweight timeline view; valid while the source activity is alive and unmodified.

### `ActivityVisitor` (core/ActivityVisitor.h)
`visit(const Activity&) / visit(const Task&) / visit(const Meeting&)` = 0 (solo 3 overload, non 5). Implementato da `JsonActivityVisitor` in `model/persistence/` per la serializzazione.

### Concrete classes (core/ + domain/)
- `Activity(title, start, duration = 0, generator = nullptr, end = TimePoint::max())` — throws `std::invalid_argument` on negative duration (ctor and `setDuration`). Rappresenta sia l'evento singolo (generator = `SingleGenerator`) sia una serie ricorrente (generator = `FixedIntervalGenerator`/`MonthlyGenerator`/`YearlyGenerator`).
- `Task(title, due, duration = 0, priority = Medium, generator = nullptr, end = max)` — `isOverdue(tp, now)`/`isDone(tp)` per-occorrenza (`m_doneOccurrences`, un `unordered_set<TimePoint>`) oltre agli alias senza `tp` per l'occorrenza a `getStart()`; `timeRemaining(tp, now)`, `priorityLabel(Priority)` static (display only), `isCheckable()/isChecked()/setChecked()`.
- `Meeting(title, start, duration = 0, location = "", generator = nullptr, end = max)` — `getLocation/setLocation`, `addAttendee/removeAttendee/attendeeCount/getAttendees` (niente duplicati).

### `Calendar` (domain/Calendar.h)
Owns the heterogeneous activities (`vector<unique_ptr<Activity>>`). `add(unique_ptr<Activity>)` (throws on null, ritorna `Activity&`), `remove(const Activity*)` (by identity), `find(const Activity*)` (const/non-const overload, ricerca per identità), `pop(const Activity*)` (rimuove e restituisce l'`unique_ptr`, `nullptr` se non trovata), `clear()`, `size()/empty()`, `occurrencesIn(from,to)` (aggrega tutte le attività, ordinate per `start` con `std::ranges::sort`), `search(needle)` (case-insensitive title substring, empty = all), const `begin()/end()` iteration (for persistence). Supporta move-assignment (usato da `persistence::loadFromFile`).

### Costruzione: Config + factory (`builders/ActivityConfig.h`), non Builder
Non esistono classi `ActivityBuilder`/`TaskBuilder`/`MeetingBuilder`/`GeneratorBuilder`. Al loro posto, struct aggregate con default espliciti — `ActivityConfig{title, start, duration, end, generator, exceptions}`, `TaskConfig : ActivityConfig {priority, done}`, `MeetingConfig : ActivityConfig {location, attendees}` — passate a funzioni libere `makeActivity(ActivityConfig)`, `makeTask(TaskConfig)`, `makeMeeting(MeetingConfig)` che restituiscono l'`unique_ptr<Activity>`/`unique_ptr<Task>`/`unique_ptr<Meeting>` corrispondente (usate sia dai test sia da `app/`). Il generatore di default (nullptr → `SingleGenerator`) è gestito dal costruttore di `Activity`, non dalla config.

### `Format` (core/Format.h)
Header-only display helpers: `formatDateTime(TimePoint)` → "YYYY-MM-DD HH:MM" UTC, `formatDuration(Duration)` → "1g 2h 30m", più `formatIso8601`/`parseIso8601` (strict, 19-char `YYYY-MM-DDTHH:MM:SS`) usati dalla persistenza.

### Generators (generators/) — stateless, senza `setStart`/`setEnd`
- `SingleGenerator()` — nessuno stato; `align` restituisce `start` se `start >= from`, altrimenti `TimePoint::max()`; `next` restituisce sempre `TimePoint::max()`.
- `FixedIntervalGenerator(Duration interval)` — passo fisso; `getInterval()`.
- `MonthlyGenerator(int months = 1)` — passi di mesi CALENDARIALI con clamping (31/1→28/2); `getMonths()`.
- `YearlyGenerator(int years = 1)` — passo annuale sul mese/giorno di `start`; Feb 29 → Feb 28 fallback negli anni non bisestili; `getYears()`.
- Tutti implementano `next(current)`, `align(start, from)` (O(1)), `accept(DateGeneratorVisitor&)` e l'interfaccia `utils::Cacheable` (`hash()`/`isEqualImpl()`, usata per un futuro pool di deduplicazione — non ancora presente). `DateGenerator::occurrences(start, from, limit)` è il metodo non-virtuale, condiviso da tutti, che genera pigramente le date (coroutine `std::generator<TimePoint>`, definito in `DateGenerator.cpp`).

## Design decisions & invariants

1. **Ranges are inclusive on both ends** everywhere (`DateGenerator::occurrences`, `Activity::occurrencesIn`, `Calendar::occurrencesIn`). To get N weekly occurrences from a recurrence starting at `start`, query `[start, start + (N-1)*1_weeks]`.
2. **Exceptions live on the activity, not on the generator.** Decorators and provider machinery were removed long ago — do not reintroduce them (this includes `MaxOccurrencesDecorator`, which no longer exists).
3. **`Activity` accepts any `DateGenerator`** via `shared_ptr<const DateGenerator>`; the generator itself is immutable, so shared ownership is safe (no aliasing-mutation hazard like the old typed-`shared_ptr` design).
4. `Activity::getEnd() == m_end` (indipendente da `start+duration`, quello è il limite della serie); negative durations throw in the ctor and in `setDuration`.
5. **No `getType()`-style string dispatch**: type-dependent behavior goes through the Visitor (double dispatch) or virtual methods (`describe()`, `clone()`, `accept()`). Type strings exist only for display/persistence (`"type"` JSON field: `event|task|meeting`).
6. `Schedulable`, `GroupSchedulable<T>`, `Events`, `EventFactory`, `Event`, `RecurrentEvent`, `Anniversary` and any Builder classes were **all removed** — do not reintroduce them. `server/`/`db/` (the REST server that used to depend on `Event`/`RecurrentEvent::getSchedulable`) were removed from the repository entirely; there is no external module currently depending on the model's public API besides `app/`.
7. JSON persistence lives in `model/persistence/` (library `events_persistence`, namespace `persistence`, Qt Core only): the `events` library itself stays pure C++23 and keeps compiling without Qt Widgets. Model **tests use QTest** (Qt Test module), not Catch2 (see Gotchas).


# Summary project intentions

> NOTE: this is the original project specification, kept as reference for the roadmap (server, client, persistence). It does NOT match the current code in several ways: `Exception`/`Modification` domain classes and `RecurrenceStrategy` are **not implemented** (exceptions are `TimePoint`s in a set; modification is a client-side flow); `Event`/`RecurrentEvent`/`Anniversary` **do not exist** (the model has only `Activity`/`Task`/`Meeting`, see "Current Model Logic"); and the entire **`server`/database macro-area below was built, then removed from the repository** (`server/`, `db/` no longer exist — commit "rimosso stack db/server e servizi docker-compose collegati"). The `ActivityVisitor` interface IS implemented and used by JSON persistence (`model/persistence/`); the Qt client is the standalone `app/`, not a REST client. The implemented model is described in "Current Model Logic" above.

Here is the updated specification document, translated into English and with the `/api/events` endpoint modified to include the mandatory `from` and `to` time range parameters.

---

# System Specification and Architecture Document

## Overview

The project adopts a distributed and modular architecture, strictly separating the business logic from the user interface and data persistence. The system is divided into **three independent macro-areas** that communicate via clear interfaces and standard protocols:

1. **C++ Model:** Pure domain logic (event management).
2. **API Server & Database:** C++ backend for handling requests and data persistence on PostgreSQL.
3. **Qt Client:** Desktop graphical user interface (GUI) that consumes the APIs.

---

## 1. Macro-Area: Model (Domain Logic)

This is the "core" of the application. This C++ library contains the business rules and event modeling, remaining completely agnostic of the database, server, or graphical interfaces.

* **Domain Classes:** Hierarchical implementation of events (`Event`, `RecurrentEvent`, `Exception`, `Modification`, `Events`(list of events that has method getSchedulable(from, to))) based on `std::chrono`.
* **Visitor Pattern for Persistence:** To strictly adhere to the *Single Responsibility Principle*, event classes will not contain any SQL or JSON logic. Instead, the **Visitor Pattern** will be implemented:
* An `EventsVisitor` interface is defined.
* Event classes expose an `accept(EventVisitor& v)` method.
* A `DatabaseSaveVisitor` implements the concrete extraction logic. When it "visits" an event, it extracts the necessary data and generates the query or payload. This allows adding new export/save formats in the future without ever modifying the core event classes.



---

## 2. Macro-Area: API Server and Database

A separate C++ executable acting as a bridge between the database and the clients. It does not process complex temporal logic (which is delegated to the Model) but securely exposes resources.

### RESTful API Design

The APIs use the HTTP protocol and exchange data exclusively in **JSON** format. Security is enforced via stateless **JWT (JSON Web Tokens)**.

| Endpoint | Method | Authentication | Description |
| --- | --- | --- | --- |
| `/api/login` | **POST** | None | Receives credentials, returns the JWT token. |
| `/api/events?from={date}&to={date}` | **GET** | Bearer Token | Retrieves the list of events for the user. **Mandatory parameters:** `from` and `to` are required to filter the timeline, preventing the server from querying and returning the entire database at once. |
| `/api/create-event` | **POST** | Bearer Token | Creates a new event (single or recurrent specified in json). |
| `/api/events/{id}` | **DELETE** | Bearer Token | Deletes an event or adds an exception to a recurrent sequence. |

### Database Management

* **Engine:** PostgreSQL. Chosen for its relational robustness and referential integrity.
* **Connection Pool:** The server implements a thread-safe connection pool to handle parallel HTTP requests without causing bottlenecks or concurrency crashes.

Tables:

Utenti
Nome | HashPassword

Compleanno

---

## 3. Macro-Area: Qt Client

The desktop application developed in **Qt (C++)**. The client is completely "dumb" regarding database logic: it never executes direct SQL queries.

* **Network Communication:** Uses `QNetworkAccessManager` to make asynchronous HTTP calls to the API Server, avoiding any blocking on the main GUI thread.
* **Authentication Handling:** Upon successful login, the Client stores the JWT token in memory and automatically injects it into the `Authorization: Bearer <token>` header of every subsequent request.
* **Internal Architecture:** Implements the **Model-View-Controller (MVC)** or **Model-View-ViewModel (MVVM)** pattern to map the JSON payloads received from the APIs into C++ objects displayed in Qt widgets (calendars, lists, forms).

---

## Selected Libraries

To ensure stability and avoid reinventing the wheel, the project relies on the following technology stack:

| Component | Selected Library | Purpose |
| --- | --- | --- |
| **HTTP Server** | `cpp-httplib` | Lightweight, multithreaded, header-only framework to expose the APIs. |
| **Serialization** | `nlohmann/json` | Intuitive conversion from C++ objects to JSON strings and vice versa. |
| **Authentication** | `jwt-cpp` | Creation, signing, and validation of JWT tokens on the server. |
| **Database** | `libpqxx` | Official C++ library for native PostgreSQL interfacing. |
| **UI Client** | `Qt6` (or Qt5) | Cross-platform graphical interface and HTTP network client. |
| **Testing** | `Catch2` | Framework for deterministic and fast unit testing on the domain Model. |

---

## Infrastructure and Containerization (Docker)

The backend infrastructure is separated and orchestrated via `docker-compose`, guaranteeing an identical environment for both development and testing/production.

1. **DB Container (`event_db`):**
* Based on the official `postgres:15-alpine` image.
* Exposes port `5432` only within the internal Docker network.
* Uses Docker volumes (`db_data`) to ensure data persistence across reboots.


2. **API Container (`api_server`):**
* Compiles the C++ Server source code into an image based on Ubuntu or Alpine.
* Exposes port `8080` to the host machine.
* Configured to wait for the DB container to be fully initialized before starting its own routines (using `depends_on`).



*(Note: The Qt Client is not containerized. It is compiled and run natively on the local machine as a standard desktop application).*
