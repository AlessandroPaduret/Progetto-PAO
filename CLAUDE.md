# AGENTS.md

C++20 library for calendar/personal activities (domain model). Namespace `events` (lowercase). The whole logical model lives in `model/`; the repo root only holds the Dockerfile (builds the model), README, LICENSE and this file.

> **Git workflow:** ogni volta che vengono fatte modifiche al codice, va creato un commit (branch corrente: `refactor`). Commit atomic, messaggi concisi in italiano, coerenti con lo stile del log esistente. Non fare push/PR a meno che non sia esplicitamente richiesto.

> **Delivery target (PAO course spec 2025/26):** the app to be delivered must compile in the professor's container (`unipd-oop/qt-env:2025`, Ubuntu 24.04 + g++ 13.3 + cmake 3.28 + **Qt 6.4.2 + qmake 3.1**, NO Catch2/nlohmann/libpqxx) and run **standalone** with local JSON file persistence. The REST server + PostgreSQL stack is an optional extra, off the evaluation critical path. Roadmap: Fase 0 (env check) + Fase 1 (Activity hierarchy) + Fase 2 (JSON persistence via Visitor) + Fase 3 (standalone Qt single-window app) + Fase 4 (delivery: `Attivita.pro` qmake, `examples/attivita_esempio.json`, `pack_delivery.sh` zip script, verified build-from-zip in the prof container) + Fase 5 (PDF report: `relazione/relazione.tex`, compiles 4pp) DONE. Remaining for the student: UML figure (`relazione/figura/`), name/matricola, actual hours in the report table.

## Refactoring in corso (branch `refactor`) — paletti + TODO

> Stato intermedio: il modello è stato ridisegnato e `app/` è stato allineato (compila); `server/` e `db/` NON sono ancora aggiornati e quindi non compilano. Scopo del refactor: `model/` + `model/persistence/` (+ `app/`).

## Refactoring completato (branch `refactor`) — Architettura finale + TODO

> **Stato attuale:** Il modello (`model/`), la persistenza JSON (`model/persistence/`) e l'applicazione Qt (`app/`) sono stati completamente rifattorizzati e **compilano/passano i test con successo**. L'architettura dei generatori è ora **stateless, immutabile e deduplicata** (Flyweight Pattern).

- [x] **`Activity` concreta & derivati (`Task`, `Meeting`)**:
  - `Activity` possiede lo **stato dell'evento**: `m_title`, `m_start`, `m_end` (default: `TimePoint::max()`), `m_duration`, `m_maxOccurrences` (0 = illimitate), le eccezioni (`m_exceptions`) e un puntatore condiviso immutabile al generatore (`std::shared_ptr<const DateGenerator>`).
  - `title` e `start` sono **obbligatori** nei costruttori di `Activity`, `Task` e `Meeting` (senza parametri di default ambigui).
  - `Task` e `Meeting` sono sottoclassi di `Activity`. `Task` gestisce il completamento per-occorrenza (`m_doneOccurrences`).
  - `Activity` supporta copia e assegnamento in $O(1)$ (Value Semantics) e clonazione tramite `clone()`.
- [x] **DateGenerator STATELESS & IMMUTABILI**:
  - `DateGenerator` è una pura regola di calcolo matematico/calendariale (senza `m_start` né `m_end`).
  - Interfaccia: `next(TimePoint current)` e `align(TimePoint start, TimePoint from)` per calcoli algoritmici in $O(1)$ senza cicli d'espansione.
  - Implementa l'interfaccia **`utils::Cacheable`** (`isEqualImpl` + `hash`) e il Visitor pattern (`accept(DateGeneratorVisitor&)`).
  - Classi concrete: `SingleGenerator` (stateless, `next` restituisce `TimePoint::max()`), `FixedIntervalGenerator(interval)`, `MonthlyGenerator(months)` (leap/end-of-month clamping aware), `YearlyGenerator(years)` (29/2→28/2 fallback).
- [] **Deduplicazione via Flyweight Pattern (`GeneratorPool`)**:
  - La `GeneratorPool` gestisce la deduplicazione in memoria dei generatori tramite `std::unordered_set<std::weak_ptr<const DateGenerator>, WeakCacheableHash, WeakCacheableEqual>`.
  - Se due attività usano la stessa regola (es. "ogni 24 ore"), condividono lo stesso indirizzo di memoria RAM. La pulizia dei nodi non più usati dalle attività avviene automaticamente quando il `shared_ptr` scade (`ref count == 0`).
- [] **Builder aggiornati**:
  - `ActivityBuilder`, `TaskBuilder` e `MeetingBuilder` accettano `title` e `start` nel costruttore e permettono configurazioni fluenti (`withEnd`, `withDuration`, `addGenerator`, `withMaxOccurrences`, `addException`). Fallback: `SingleGenerator` condiviso statico.
- [] **Persistenza JSON (`JsonPersistence`)**:
  - Adeguata al Visitor pattern aggiornato: i generatori serializzano solo la propria frequenza/regola (`type`, `interval_seconds`, `interval_months`, `interval_years`), mentre `start`, `end` e `max_occurrences` sono serializzati e deserializzati al livello dell' `Activity`.
  - Deserializzazione tramite `std::shared_ptr<const DateGenerator>`.
- [ ] `server/`, `db/`, `examples/*.json` — da aggiornare in un secondo momento (fuori scope attuale). `app/` compila (viste e form aggiornati ai builder; `ActivityFactory`, `MoveGeneratorVisitor` e `truncateBefore` non esistono piu').

### 📋 TODO: Inserimento Pipeline C++20 Ranges (`std::ranges::view_interface`)

Rifattorizzare il calcolo e la materializzazione delle occorrenze in `Activity::occurrencesIn` tramite la composizione pigra (lazy evaluation) di **C++20 Ranges**:

- [ ] **Implementazione di `OccurrenceRange`**:
  - Definire `OccurrenceRange` derivandolo da `std::ranges::view_interface<OccurrenceRange>` per avvolgere `GeneratorIterator(gen, from, to)`.
- [ ] **Pipeline lazy per filtraggio eccezioni e limiti**:
  - Sostituire l'espansione manuale in `Activity::occurrencesIn` con una composizione fluente di range views:
    ```cpp
    OccurrenceRange range{m_generator.get(), from, std::min(to, getEnd())};

    // 1. Filtraggio eccezioni via std::views::filter
    auto validDatesView = range | std::views::filter([this](TimePoint tp) {
        return !m_exceptions.contains(tp);
    });

    // 2. Troncamento limitato via std::views::take (se m_maxOccurrences > 0)
    // auto limitedView = validDatesView | std::views::take(m_maxOccurrences);
    ```
  - Materializzare il `std::vector<TimePoint>`/`std::vector<Occurrence>` finale iterando sulla pipeline costruita in un unico passaggio lazy.
- [ ] **Allineamento moduli esterni**:
  - Aggiornare `server/` e `db/` in un secondo momento per allinearli ai nuovi tipi di `model/`.

## Build & test

CMake is the primary build. Run from `model/` (the `model/Dockerfile` installs all deps, incl. Catch2):

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
  - `model/include/events/` — public headers; code includes them as `events/events.h` (require `-Iinclude`).
  - `model/src/events/` — implementation (matches headers 1:1).
  - `model/tests/test.cpp` + `model/tests/test_persistence.cpp` — the test files (Catch2 v3).
  - `model/persistence/` — JSON persistence module, library `events_persistence` (Qt Core ONLY, no Widgets): `persistence::activityToJson/calendarToJson/activityFromJson/calendarFromJsonArray/saveToFile/loadFromFile`. Serialization uses the Visitor pattern (`JsonActivityVisitor` + `JsonGeneratorVisitor`); deserialization is a discriminator-based factory ("type" field `event|recurrent|task|meeting|anniversary`, validated strictly). File format: `{"version":1, "activities":[...]}`, times as ISO-8601 UTC `YYYY-MM-DDTHH:MM:SS`. Solo il **Task** serializza il flag `done`; Meeting `location`+`attendees`, Task `priority`. Built only when Qt6 Core is found (present on host AND in the professor's container; NOT in the server's Dockerfile).
  - `model/Doxyfile` + `model/docs/` — Doxygen config (docs/ is gitignored).
  - `model/Dockerfile` + `model/.dockerignore` — builds the tests and generates the docs (installs qt6-base-dev so persistence tests run in CI).
  - CMake: `find_package(Catch2 3 QUIET)` — tests are built ONLY when Catch2 is available (it is NOT in the professor's container; the library itself must always compile there). `find_package(Qt6 COMPONENTS Core QUIET)` gates the persistence module.
- `db/` — persistence layer, library `db_repository` (no dependency on the model):
  - `db/include/db/` — public headers: `ConnectionPool` (thread-safe pool of libpqxx connections, RAII `Lease`), `PasswordHasher` (bcrypt via libxcrypt, `$2b$`, cost 12, self-generated salt), `UserRepository`, `EventRepository`.
  - `db/src/db/` — implementations on libpqxx. Timestamps are epoch-seconds (BIGINT) to match the model's seconds precision.
  - `db/schema.sql` — DDL (tables `utenti`, `eventi` with `SINGLE|FIXED|YEARLY`, `eccezioni`), applied at container init.
  - `db/tests/` — `hash_smoke` (bcrypt, no DB; registered in ctest) and `repo_smoke` (needs a live PostgreSQL via `DATABASE_URL`; not in ctest).
  - `db/Dockerfile` + `db/.dockerignore` — reproducible build env: installs `libpqxx-dev` + `libcrypt-dev`, builds the library and runs the smoke tests.
- `server/` — REST API server, executable `api_server` (links `events` + `db_repository`):
  - `server/src/main.cpp` — cpp-httplib routes + Bearer middleware.
  - `server/src/auth.*` — JWT (jwt-cpp, HS256, `sub`=userId, exp 24h).
  - `server/src/mappers.*` — `EventRecord` → `Event`/`RecurrentEvent` (the db filters candidates, the model expands occurrences). Note the ns→s `time_point_cast` conversion (db uses `system_clock::time_point`, the model uses seconds).
  - `server/src/iso8601.*` — ISO-8601 (UTC) parse/format ↔ `events::TimePoint`. Strict parser (`sscanf` + `year_month_day::ok()`): rejects partial dates like `2026-01-01`.
  - `server/tests/` — Catch2 unit tests (no DB): `test_time`, `test_auth`, `test_mappers`; registered as one ctest entry `server_tests`.
  - `server/CMakeLists.txt` — `add_subdirectory(../model)` + `(../db)` with `BUILD_TESTING OFF`; library `api_core` (auth/mappers/iso8601, testable) + executable `api_server`; deps: `nlohmann_json` (apt) + `cpp-httplib` and `jwt-cpp` (FetchContent, pinned tags — cpp-httplib must be ≥ 0.19 for GCC 13.2).
  - `server/Dockerfile` — build context = repo root (needs `model/` + `db/`); builds + runs `server_tests` during image build; root `.dockerignore` excludes artifacts.
- `app/` — Qt6 desktop app **standalone** (single-window, deliverable PAO; native build, NOT containerized; MVC: `CalendarController` + views):
  - `app/src/CalendarController.*` — controller MVC (QObject): possiede `events::Calendar`, CRUD + `search` + `occurrencesIn`, `addActivities` (piu' serie in un colpo), azioni sulle occorrenze (`deleteOccurrence` → EXDATE o truncate "questa e le successive", `modifyOccurrence` → eccezione interna + evento singolo, `updateActivity` conserva le eccezioni), **`toggleDone(Occurrence)`** (inverte lo stato evaso/da fare solo di un COMPITO, l'unico tipo con stato), persistenza via `events_persistence`. Segnale `activitiesChanged` → le viste si aggiornano.
  - `app/src/views/` — viste Qt (MVC, mai path cablati):
    - `MainWindow` — finestra unica (vincolo PAO) con QStackedWidget a **5 pagine**: 0=settimana, 1=elenco, 2=giorno, 3=mese, 4=anno. **QMenuBar** in alto (GUI tipica) con i menu: **File** (Salva calendario **Ctrl+S** — salva sull'ultimo file usato, altrimenti chiede il nome / Salva con nome **Ctrl+Shift+S** / Carica calendario **Ctrl+O**, via QFileDialog; le scorciatoie sono disegnate a SINISTRA della voce in grigio/trasparente tramite lo stile `LeftShortcutStyle`), **Visualizza** (Elenco/Giorno/Settimana/Mese/Anno, azione attiva spuntata con QActionGroup) e **Nuova attivita'** (Evento/Riunione/Compito/Anniversario → `openNewActivityType` → `ActivityFormDialog::startCreateType`, preseleziona il tipo nel form; **Ctrl+N** apre il form con tipo Evento). **Barra di navigazione condivisa** Oggi/<- /->/etichetta estratta fuori dallo stack (nascosta su Elenco): sposta la `QDate` di riferimento `m_anchor` (giorno:+-1, settimana:+-7, mese:+-1 mese, anno:+-1 anno; `setAnchor` la normalizza per la vista corrente: lunedi' / 1 del mese / 1 gennaio). Account per vista: `ViewKind{Day,Week,Month,Year}`. Dettaglio non ha spunta nel menu. Pannelli interni `ActivityDetailDialog`, `ActivityFormDialog` + `RecurrenceChoiceDialog` (widget figli, ricentrati in `resizeEvent`).
    - `ViewShared.h` — helper condivisi inline: `activityColor` (paletta stabile dall'indirizzo dell'oggetto), `localTime` (UTC->locale), `shortDayName(dayOfWeek)`.
    - `WeekView` — griglia settimanale QPainter, **numero di giorni configurabile** (`setDayCount`, default 7; `dayWidth()/hourHeight()` scalano): ore sul bordo, **striscia dedicata "tutto il giorno"** in alto (chip delle occorrenze che coprono un giorno intero — `coversFullDay`, spanning su piu' date), **eventi sovrapposti affiancati a colonne** come Google Calendar, durata zero = chip da 18px; **spunta (checkbox) solo sui Compiti** → `doneToggled` → `CalendarController::toggleDone` (compiti evasi attenuati con spunta spuntata); **anteprima live** `Preview{title,start,duration}` aggiornata a ogni modifica dei campi del form e rimossa alla chiusura; **drag&drop**: tenere premuto un'occorrenza e rilasciarla su una nuova cella emette `activityMoved` -> `CalendarController::moveActivity` -> `Activity::setStart` + `clearExceptions`; per i ricorrenti lo spostamento NON trasla la fine della serie (resta la stessa; se il nuovo inizio supererebbe la fine, la fine viene portata al nuovo inizio per non restare antecedente) e la serie viene ricreata INTONSA (le eccezioni sono svuotate: gli eventi staccati restano come eventi singoli indipendenti, la serie spostata non ha buchi); segnale `activityEditRequested` dal doppio clic per modificare l'attivita' intera conservando il tipo; menu contestuale: per i ricorrenti "Modifica" (serie) + "Modifica istanza" (singola occorrenza), per gli altri "Modifica" dell'attivita'. Doppio clic/drag su un'occorrenza successiva alla prima della serie apre `RecurrenceChoiceDialog` (pannello interno: "Modifica tutta la serie" -> form serie precompilato / "Modifica solo questo evento" -> form evento standard, al salvataggio la serie continua senza quel giorno).
    - `DayView` — sottoclasse di `WeekView` con `setDayCount(1)`: stessa griglia, striscia all-day, drag&drop, spunte, anteprima e menu contestuale ma a colonna singola (ore piu' ampie).
    - `MonthView` — griglia mensile 7x5-6 QPainter: numero del giorno (oggi evidenziato, fuori-mese in grigio) e **chip colorati** (max 3/giorno + "+N"); ogni chip dei **Compiti** ha la **spunta** → `doneToggled`; clic sinistro seleziona, doppio clic modifica (con scelta serie/istanza per i ricorrenti, come in WeekView), doppio clic su cella vuota crea alle 09:00, menu contestuale, tooltip; niente drag&drop ne' anteprima live.
    - `YearView` — 12 mini-calendari in griglia 4x3 (un mese ciascuno, nomi giorni + numeri) con pallini colorati per le attivita' (grigi se Compito evaso); doppio clic su un giorno -> `daySelected` -> la MainWindow passa alla vista giorno di quella data; tooltip con i titoli del giorno.
    - `ActivityListPage` (tabella a 4 colonne: **pallino del colore dell'attivita'** (come le griglie del calendario) + Titolo/Tipo/**Dettagli**; ricerca live per titolo via `Calendar::search` + **filtro per tipo** (QComboBox "Tutti i tipi"/Evento/Ricorrente/Riunione/Compito/Anniversario) + **checkbox "Solo da fare"** (nasconde i Compiti evasi); ordinamento a clic sull'intestazione: su "Titolo" ordina per titolo con "Tipo" come chiave secondaria (e viceversa su "Tipo"; default per data di inizio), indicatore di sort mantenuto tra i refresh; stile pulito: fondo bianco, righe alternate, header a contrasto, **separatori verticali grigi tra le colonne**, **testo nero 13pt e righe alte per leggibilita'**). `ActivityDetailDialog` (pannello ridotto DENTRO la MainWindow — widget figlio, non esce dai bordi: boundedTo(host), ricentrato in `MainWindow::resizeEvent`; titolo 18pt centrato rispetto alla finestra (margine sinistro pari alla larghezza della "X") e righe "campo: valore" 13pt centrate e distanziate come paragrafi; "X" (fuori dalla riga del titolo, in alto a destra dell'angolo) per chiudere + pulsanti in basso "Modifica" -> `editRequested` (apre il form) e "Elimina").
    - `ActivityFormPage` (form per tipo in QStackedWidget: **Evento "a domande"**/Riunione/Compito/Anniversario; modi Create/EditActivity/EditOccurrence). Il pannello **Evento NON chiede il tipo**: chiede titolo, data (slot separati), ora, durata e le checkbox **"Si ripete?"** e **"Tutto il giorno"**. Se si ripete: unita' (giorni/settimane/mesi/anno), "ogni N", per le **settimane** i **giorni della settimana** (pulsanti cliccabili Lun..Dom, si colorano) e la **fine** (radio: Mai / Fino al [data] / Dopo N occorrenze). La checkbox **"Tutto il giorno"** nasconde ora e durata e istanzia un `Event` dalle 00:00 di 24h (o, con "Si ripete", una serie che ricorre a giornate intere): la GUI mostra queste occorrenze nella striscia in alto. In base alle risposte istanzia un `Event` oppure uno o piu' `RecurrentEvent` (**uno per giorno della settimana** selezionato, "ogni lun e ven" = 2 serie settimanali). "Ogni N mesi" usa `MonthlyGenerator`; "fino a" imposta `end`, "dopo N" `GeneratorBuilder::limitTo`. Il pannello Riunione ha Luogo + elenco Partecipanti (Aggiungi con Invio / Rimuovi con doppio clic); campi propri per pannello in QGridLayout etichetta a sinistra/box affiancata, titolo e data conservati al cambio tipo via `syncCommonFields` — niente widget condivisi tra i pannelli. `CalendarController::addActivities` aggiunge piu' serie in un colpo solo.
  - `app/tests/test_controller.cpp` — test Catch2 del controller (headless, in ctest).
  - `app/CMakeLists.txt` — Qt6 Widgets+Core, AUTOMOC; `add_subdirectory(../model)`; target `pao_core` (controller, testabile senza Widgets) + `pao_app` + `app_controller_tests` (solo se Catch2).
  - Build: `cmake -B build && cmake --build build -j && ctest --test-dir build` da `app/`. Compila nel container del professore (Qt 6.4.2): attenzione a NON usare API Qt ≥ 6.5 (es. `QTimeZone::UTC` non esiste lì — usare `QTimeZone(0)`); `QDateTime` UTC via `QTimeZone(0)` o `Qt::UTC` (deprecato solo su Qt recenti).
- `docker-compose.yml` (repo root) — services `model` (test+docs pipeline), `db` (postgres:15-alpine, schema auto-init, volume `db_data`, port 5432 NOT exposed, network `pao-backend`) and `api` (builds `server/`, waits for `db` health, exposes 8080).
- **Delivery artifacts** (repo root): `Attivita.pro` (qmake single-target app build, verified with `qmake6 && make` in the prof container), `examples/attivita_esempio.json` (sample persistence file, generated and round-trip validated by `events_persistence`), `relazione/relazione.tex` (PAO report, 10pt ≤8pp, compiled with pdflatex; UML placeholder in `relazione/figura/uml_attivita.png` — to be drawn by the student; student name/matricola via `\studente`/`\matricola`), `pack_delivery.sh` (builds the Moodle zip: `relazione.pdf` auto-included if present + `sorgenti/` with model+app+`.pro` + `examples/`; excludes server/db, build artifacts, docs). Verified end-to-end: unzip in clean dir inside `qt-env-prof:2025` → `qmake6 && make` (0 errors/warnings) and `cmake -B build -DBUILD_TESTING=OFF && cmake --build build` → app runs offscreen.

DB semantics: `EventRepository::getEvents(userId, from, to)` filters in SQL — SINGLE events by `start ∈ [from,to]`; FIXED/YEARLY by `start <= to AND (fine IS NULL OR fine >= from)` (the precise occurrence expansion is delegated to the model's `getSchedulable`). `setRecurrenceEnd(eventId, userId, end)` truncates a FIXED/YEARLY recurrence (UPDATE `fine`, refused on SINGLE).

API: all routes except `/api/login` and `/api/register` require `Authorization: Bearer <token>`. Dates in requests/responses are ISO-8601 UTC (`YYYY-MM-DDTHH:MM:SS`). Endpoints: `POST /api/register` (`{username,password}` → 201/409), `POST /api/login` (→ `{token}`/401), `GET /api/events?from=&to=` (→ `[{event_id,title,start,end,type}]` occurrences, `type` is `single|fixed|yearly`, 400 without valid from/to), `POST /api/create-event` (`{title,start,duration,type:"single|fixed|yearly",interval?,end?}` → 201 `{id}`/400), `DELETE /api/events/{id}` — body `{"exception":ISO}` adds an EXDATE (internal), `{"truncate":ISO}` ends the recurrence just before that occurrence (only for recurrent events), no body deletes the event; 404 if not owned. Server config via env: `DATABASE_URL`, `JWT_SECRET`, `PORT` (default 8080).

## Gotchas

- `FixedIntervalGenerator::generateDates` / `Activity::occurrencesIn` ranges are INCLUSIVE on both ends (`while (current <= to)`). To get N weekly occurrences from a recurrence starting at `start`, query `[start, start + (N-1)*1_weeks]`.
- I test del modello (`tests/test.cpp`, `tests/test_persistence.cpp`) NON definiscono un proprio `main()`: il target CMake linka `Catch2::Catch2WithMain`. (Nota: `app/tests/test_controller.cpp` definisce il proprio `main` via `Catch::Session` e linka `Catch2::Catch2`.)
- Calendar literals like `2026y/2/28` compile under `-std=c++20` without extra flags; `_weeks`/`_years` literals live in `namespace events` (`CommonTypes.h`). There is NO `_days` literal — use `Days(n)`.
- In `server/`, headers live in `server/src/` which is on the include path: do NOT name one `time.h` (it shadows `<time.h>` included by `<ctime>` → circular include, cryptic compile errors). The ISO-8601 helpers are `iso8601.h`.
- `cpp-httplib` < 0.19 does not compile with GCC 13.2 (Ubuntu 24.04); the CMake pins v0.20.0. Regex route captures are exposed via `req.matches` (v0.20.0 handlers take `(req, res)` only).
- Library is header+source with no Qt dependency; the `model/Dockerfile` installs only build tools + Catch2 + doxygen + qt6-base-dev to compile/test the model in `model/` and generate `model/docs/`.
- **Professor's delivery container** (`qt-env-prof:2025`, reproduced from `unipd-oop/qt-env:2025` Dockerfile): Ubuntu 24.04, g++ 13.3, cmake 3.28.3, Qt 6.4.2 + qmake 3.1 (qt6-base/svg/charts/multimedia/declarative), libsqlite3; NO Catch2, NO nlohmann-json, NO libpqxx. The `events` library MUST compile there (Catch2 optional). GUI code must stay Qt ≤ 6.4 compatible. The server/db stack can only build natively or via its own Dockerfile (network needed for FetchContent).


# Current Model Logic (implemented)

Namespace `events`. Abstractions in `model/include/events/core/`, concrete classes in `model/include/events/domain/` and `model/include/events/generators/`, implementations 1:1 in `model/src/events/`. Entry point: `events/events.h` (relative to `model/include/`).

## Class hierarchy

```
Activity (abstract)                         // root of the activity hierarchy (title + id-less)
├── Activity                                   // single interval: title + start + duration
├── Task                                    // Compito: due + Priority{Low,Medium,High} + done (unico con stato)
└── Meeting                                 // Riunione: interval + location + attendees
DateGenerator (abstract)                    // produces occurrence timestamps (Strategy)
├── FixedIntervalGenerator                  // start, interval (giorni/settimane), end (mutabili)
├── MonthlyGenerator                        // start, passi di MESI di calendario, end, months (mutabili)
└── YearlyGenerator                         // yearly, leap-year aware
Calendar                                    // OWNS the polymorphic activities (vector<unique_ptr<Activity>>)
```

The hierarchy has **5 concrete activity types** (course spec requires ≥3), each with genuinely different attributes/methods. Solo **Task** ha uno stato di completamento ("evaso/da fare", `isDone()/setDone()`); gli altri tipi non hanno stato. `Activity::addException/clearExceptions`, `Task::isOverdue/priorityLabel`, `Meeting::addAttendee/removeAttendee`.

Il **limite di occorrenze** ("Dopo N occorrenze", 0 = illimitate) vive nel `MaxOccurrencesDecorator` (`GeneratorBuilder::limitTo` / `ActivityBuilder::withMaxOccurrences`) e viene serializzato come `max_occurrences`. `MonthlyGenerator` fa passi di mesi CALENDARIALI (il 31 clampa all'ultimo giorno del mese, 31/1→28/2).

> **Nota (all-day):** NON esiste piu' un tipo `AllDayEvent`. Un evento "tutto il giorno" e' un normale `Event` (o occorrenza di serie) che parte alle 00:00 e dura 24h (fino alle 00:00 del giorno dopo). La GUI decide se mostrarlo nella **striscia in alto** in base a date e durate: `coversFullDay(occ)` (ViewShared.h) e' true se l'occorrenza copre almeno un giorno di calendario intero.

### `Activity` (core/Activity.h)
Abstract root: `m_title`, `getTitle()/setTitle()`, and the non-trivial polymorphic interface:
- `occurrencesIn(from, to) -> vector<Occurrence>` — each type expands its own notion of "when it happens" (Event → 0/1, RecurrentEvent → recurrence minus exceptions, Task/Meeting → single block, Anniversary → yearly). **Range INCLUSIVE on both ends**, start-of-occurrence semantics.
- `accept(ActivityVisitor&)` — double dispatch (Visitor pattern for persistence/GUI panels).
- `describe() -> String` — per-type human summary (display only, allowed by spec vincolo 9).
- `clone() -> unique_ptr<Activity>` — virtual, override diretta in `Task`/`Meeting` (Prototype; copia profonda del generatore e, per i Task, anche delle occorrenze evase).

### `Occurrence` (core/Occurrence.h)
Value type: `{const Activity* source (non-owning), start, duration}`, `end()`, `overlaps(from,to)`. Lightweight timeline view; valid while the source activity is alive and unmodified.

### `ActivityVisitor` (core/ActivityVisitor.h)
`visit(const Event&/const RecurrentEvent&/const Task&/const Meeting&/const Anniversary&)` = 0. Used by tests (CountingVisitor); JSON persistence (Fase 2) and GUI detail panels (Fase 3) will implement it.

### Concrete classes (domain/)
- `Event(title, start, duration)` — throws `std::invalid_argument` on negative duration (ctor and `setDuration`). `getEnd() = start + duration`, `isIn` (fully contained), `overlapsWith(const Event&)`.
- `RecurrentEvent(shared_ptr<DateGenerator>, Event template)` — `m_exceptions` set; `getSchedulable(from,to)` returns **clones** of the template (legacy API, used by the REST server — do not remove); `occurrencesIn` returns lightweight `Occurrence`s. `getGenerator()/getTemplateEvent()/getExceptions()` exposed for persistence.
- `Task(title, due, Priority)` — `isOverdue(now)` (only if not done), `timeRemaining(now)`, `isDone()/setDone()` (stato proprio del Task), `priorityLabel(Priority)` static (display only). Occurrences have zero duration.
- `Meeting(title, start, duration, location)` — `getLocation/setLocation`, `addAttendee/removeAttendee/attendeeCount/getAttendees`; negative duration throws.
- `Anniversary(title, date, end = max)` — ricorrenza annuale leap-aware (29/2→28/2) riusando `YearlyGenerator` per composizione; durata all-day (24h−1s).

### `Calendar` (domain/Calendar.h)
Owns the heterogeneous activities. `add(unique_ptr<Activity>)` (throws on null), `remove(const Activity*)` (by identity), `clear()`, `size()/empty()`, `occurrencesIn(from,to)` (aggregates all types, sorted by start), `search(needle)` (case-insensitive title substring, empty = all), const `begin()/end()` iteration (for persistence).

### Builders (builders/)
Percorso di creazione raccomandato (le factory sono state eliminate): `ActivityBuilder` (con `start` e `title` obbligatori, fallback `SingleGenerator(now)` se non definisci nessun' altro generator), `TaskBuilder`, `MeetingBuilder`, `GeneratorBuilder`.

### `Format` (core/Format.h)
Header-only display helpers: `formatDateTime(TimePoint)` → "YYYY-MM-DD HH:MM" UTC, `formatDuration(Duration)` → "1g 2h 30m". Used by `describe()`.

### Generators (generators/) — limiti mutabili con setStart/setEnd
- `SingleGenerator(point)` — un'unica data (`getPoint()`); `setStart`/`setEnd` spostano il punto.
- `FixedIntervalGenerator(start, interval, end = TimePoint::max())` — occurrences every `interval` from `start`; `setStart`/`setEnd` per spostare/troncare (l'intervallo resta fisso).
- `MonthlyGenerator(start, months = 1, end = TimePoint::max())` — passi di mesi CALENDARIALI con clamping (31/1→28/2); `setMonths` cambia il passo.
- `YearlyGenerator(start, end = TimePoint::max())` — one occurrence per year on the month/day of `start`; Feb 29 → Feb 28 fallback in non-leap years.
- All generators implement `accept(DateGeneratorVisitor&)` (core/DateGeneratorVisitor.h): the persistence layer serializes the recurrence rule through a second Visitor (no `getType` strings in the model).

## Design decisions & invariants

1. **Ranges are inclusive on both ends** everywhere (`generateDates`, `occurrencesIn`, `getSchedulable`). To get N weekly occurrences from a recurrence starting at `start`, query `[start, start + (N-1)*1_weeks]`.
2. **Exceptions live on the event, not on the generator.** Decorators and provider machinery were removed long ago — do not reintroduce them.
3. **`RecurrentEvent` accepts any `DateGenerator`** via `shared_ptr`; shared ownership means a caller holding a typed `shared_ptr` can mutate the generator after event creation.
4. `Event::getEnd() == getStart() + getDuration()`; negative durations throw (same for negative `Reminder` repeat).
5. **No `getType()`-style string dispatch**: type-dependent behavior goes through the Visitor (double dispatch) or virtual methods. Type strings may exist only for display.
6. `Schedulable`, `GroupSchedulable<T>`, `Events` and `EventFactory` were **removed** in the Fase-1 restructure (absorbed/replaced by `Activity`, `Calendar`, poi dai builder `ActivityBuilder`/`GeneratorBuilder`) — do not reintroduce them. The REST server still compiles because `Event`/`RecurrentEvent` keep their public APIs (incl. `getSchedulable`).
7. JSON persistence (Fase 2) lives in `model/persistence/` (library `events_persistence`, namespace `persistence`, Qt Core only): the `events` library itself stays pure C++ and keeps compiling without Catch2 and without Qt Widgets. `Format.h` provides `formatIso8601/parseIso8601` (strict, exact 19-char `YYYY-MM-DDTHH:MM:SS`, used by persistence).


# Summary project intentions

> NOTE: this is the original project specification, kept as reference for the roadmap (server, client, persistence). Parts of it do NOT match the current code: `Exception`/`Modification` domain classes and `RecurrenceStrategy` are **not implemented** (exceptions are `TimePoint`s in a set; modification is a client-side flow); the `ActivityVisitor` interface IS implemented (see "Current Model Logic") and will be used by JSON persistence (Fase 2) and GUI detail panels (Fase 3). The implemented model is described in "Current Model Logic" above.

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
