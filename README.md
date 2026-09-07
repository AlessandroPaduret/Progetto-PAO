# Calendario — gestione di attivita' personali

Calendario scritto in C++23/Qt per la gestione di attivita' personali (eventi, eventi
ricorrenti, meeting)

## Componenti

- `model/` — modello logico (namespace `events`): `Activity`
  concreta con regole di ricorrenza (Strategy via `DateGenerator`), sottoclassi
  `Task`/`Meeting`, `Calendar`, Visitor per le operazioni esterne.
  + `model/persistence/` — persistenza JSON su file (Qt Core, Visitor).
- `app/` — applicazione Qt con vista settimanale, giornaliera, mensile, annuale, 
  elenco con ricerca, dettaglio e form di creazione/modifica,
  salvataggio/caricamento sidebar.

## Build e test

```bash
# da fare nelle cartelle app/ e model/
cmake -B build && cmake --build build 
```

L'applicazione compila anche nel container di valutazione del corso
(`unipd-oop/qt-env:2025`, Qt 6.4.2): i test Catch2 vengono compilati solo se
il pacchetto e' disponibile, il resto della build non ha dipendenze esterne.

## Documentazioni

Nella cartella `docs/` c'è la documentazione autogenerata da Doxygen basata sui
commenti del progetto. C'è anche il file `Doxyfile` che serve in caso si voglia
rigenerare la documentazione.
```bash
# comando per rigenerare la documentazione (se hai doxygen installato)
doxygen
```