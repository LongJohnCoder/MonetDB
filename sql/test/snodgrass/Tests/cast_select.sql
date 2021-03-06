  -- simultaneously
-- dates, times and timestamps
SELECT CAST('1996-02-24' AS  DATE) = DATE  '1996-02-24';
SELECT CAST('12:34:56' AS TIME) = TIME '12:34:56';
SELECT CAST(TIME '12:34:56' AS TIMESTAMP WITH TIME ZONE) = TIMESTAMP '1997-07-23 12:34:56-07:00';
--rollback;
SELECT CAST( TIME '12:34:56.123' AS TIME(6) ) = TIME '12:34:56.123000';
SELECT CAST( TIME '12:34:56.123' AS TIME(1) ) = TIME '12:34:56.1';
SELECT CAST( TIMESTAMP '1997-07-23 12:34:56.123' AS TIME(6) ) = TIME '12:34:56.123000';
SELECT CAST( TIMESTAMP '1997-07-23 12:34:56.123' AS DATE ) = DATE '1997-07-23';
SELECT CAST( DATE '1997-01-01' AS TIMESTAMP(4)) = '1997-01-01 00:00:00.0000';

-- intervals
SELECT CAST('2' AS INTERVAL MONTH );
SELECT CAST('3-7' AS INTERVAL MONTH );
-- SELECT CAST(INTERVAL '8-7' YEAR TO MONTH AS INTERVAL MONTH(2) ); -- err: exception is raised
SELECT CAST( INTERVAL '3' YEAR AS INTERVAL YEAR TO MONTH ) = INTERVAL '3-0' YEAR TO MONTH;
SELECT CAST( 103 AS INTERVAL MONTH ) = INTERVAL '103' MONTH;
-- SELECT CAST( 103 AS INTERVAL MONTH(2) ); -- err: overflow exception
SELECT CAST('2 12:34' AS INTERVAL DAY TO MINUTE );
SELECT CAST('12:34' AS INTERVAL DAY TO MINUTE ); -- err: works not
SELECT CAST( INTERVAL'86 00:00:00' DAY TO SECOND AS INTERVAL HOUR TO SECOND ) = INTERVAL '2064:00:00' HOUR TO SECOND;
SELECT CAST('86 00:00:00' AS INTERVAL HOUR(3) TO SECOND ); -- err: overflow exception
SELECT CAST( CAST( 7430400 AS INTERVAL SECOND ) AS INTERVAL DAY TO SECOND ) = INTERVAL '86 00:00:00' DAY TO SECOND;
SELECT CAST(DATE '1997-01-01' AS CHARACTER(10)) = '1997-01-01';
SELECT CAST(INTERVAL '7430400' SECOND AS BIGINT) = 7430400;
SELECT CAST(CAST(INTERVAL '2064:00:00' HOUR TO SECOND AS INTERVAL SECOND) AS BIGINT) = 7430400;
