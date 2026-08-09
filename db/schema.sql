-- Schema del database degli eventi.
-- I timestamp sono memorizzati come epoch seconds (BIGINT) per coincidere
-- con la precisione al secondo di events::TimePoint del modello.

CREATE TABLE IF NOT EXISTS utenti (
    id            BIGSERIAL PRIMARY KEY,
    nome          TEXT NOT NULL UNIQUE,
    hash_password TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS eventi (
    id         BIGSERIAL PRIMARY KEY,
    utente_id  BIGINT NOT NULL REFERENCES utenti (id) ON DELETE CASCADE,
    titolo     TEXT   NOT NULL,
    inizio     BIGINT NOT NULL,                     -- epoch seconds
    durata     BIGINT NOT NULL CHECK (durata >= 0), -- secondi
    tipo       TEXT   NOT NULL CHECK (tipo IN ('SINGLE', 'FIXED', 'YEARLY')),
    intervallo BIGINT,                              -- secondi, solo per FIXED
    fine       BIGINT                               -- epoch seconds, fine ricorrenza (NULL = illimitata)
);

CREATE TABLE IF NOT EXISTS eccezioni (
    evento_id BIGINT NOT NULL REFERENCES eventi (id) ON DELETE CASCADE,
    data      BIGINT NOT NULL,                      -- epoch seconds
    PRIMARY KEY (evento_id, data)
);

CREATE INDEX IF NOT EXISTS idx_eventi_utente ON eventi (utente_id);
CREATE INDEX IF NOT EXISTS idx_eccezioni_evento ON eccezioni (evento_id);
