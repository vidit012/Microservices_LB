CREATE TABLE IF NOT EXISTS item (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    price DOUBLE PRECISION NOT NULL
);

INSERT INTO item (name, price) VALUES
  ('iPod', 42.0),
  ('iPod nano', 50.0),
  ('iPod touch', 60.0);