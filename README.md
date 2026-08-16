# Attivita — gestione di attivita' personali

Applicazione C++20/Qt per la gestione di attivita' personali (eventi, eventi
ricorrenti, scadenze, promemoria), sviluppata per il corso di **Programmazione
a Oggetti** (Universita' di Padova, a.a. 2025/26).

## Componenti

- `model/` — libreria del modello logico (namespace `events`): gerarchia
  polimorfa `Activity` (Event, RecurrentEvent, Deadline, Reminder), generatori
  di ricorrenza (Strategy), `Calendar`, Visitor per le operazioni esterne.
  + `model/persistence/` — persistenza JSON su file (Qt Core, Visitor).
- `app/` — applicazione Qt **standalone** a finestra singola (MVC): vista
  settimanale, elenco con ricerca, dettaglio e form di creazione/modifica,
  salvataggio/caricamento via dialog.
- `db/` + `server/` — stack opzionale REST API + PostgreSQL (JWT, libpqxx),
  NON richiesto dall'applicazione standalone.

## Build e test

```bash
# modello + persistenza + test (Catch2, se presente)
cmake -B build && cmake --build build -j && ctest --test-dir build   # da model/
cmake -B build && cmake --build build -j && ctest --test-dir build   # da app/
```

L'applicazione compila anche nel container di valutazione del corso
(`unipd-oop/qt-env:2025`, Qt 6.4.2): i test Catch2 vengono compilati solo se
il pacchetto e' disponibile, il resto della build non ha dipendenze esterne.

Per lo stack opzionale: `docker compose up --build db api` (porta 8080).
