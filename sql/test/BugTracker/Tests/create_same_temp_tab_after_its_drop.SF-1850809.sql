CREATE TEMP TABLE temptest(col int) ON COMMIT DELETE ROWS;
INSERT INTO temptest VALUES (1);
DROP TABLE temptest;
CREATE TEMP TABLE temptest(col int) ON COMMIT DROP;
INSERT INTO temptest VALUES (3);

DROP TABLE temptest;
