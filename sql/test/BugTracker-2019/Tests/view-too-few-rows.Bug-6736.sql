start transaction;

CREATE SCHEMA "dw_hospital";
CREATE SEQUENCE "dw_hospital"."seq_36397" AS INTEGER;
CREATE SEQUENCE "dw_hospital"."seq_36422" AS INTEGER;
CREATE SEQUENCE "dw_hospital"."seq_45337" AS INTEGER;
CREATE SEQUENCE "dw_hospital"."seq_45407" AS INTEGER;
SET SCHEMA "dw_hospital";
CREATE TABLE "dw_hospital"."bri_classi_gruppi_movimenti" (
	"classe_movimento_id"        INTEGER       NOT NULL,
	"gruppo_classe_movimento_id" INTEGER       NOT NULL,
	"last_batch_id"              INTEGER       NOT NULL DEFAULT '0',
	"last_updated"               TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_bri_classi_gruppi_movimenti" PRIMARY KEY ("classe_movimento_id", "gruppo_classe_movimento_id")
);
CREATE TABLE "dw_hospital"."dim_classi_movimenti" (
	"id"                  INTEGER       NOT NULL,
	"codice"              VARCHAR(16)   NOT NULL,
	"descrizione"         VARCHAR(256)  NOT NULL,
	"tipo_centro"         VARCHAR(16)   NOT NULL,
	"tipo_movimentazione" VARCHAR(16)   NOT NULL,
	"tipo_caricamento"    VARCHAR(16)   NOT NULL,
	"uso_ribaltamento"    VARCHAR(16)   NOT NULL,
	"version"             INTEGER       NOT NULL,
	"valid_from"          DATE,
	"valid_to"            DATE,
	"last_batch_id"       INTEGER       NOT NULL DEFAULT '0',
	"last_updated"        TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_dim_classi_movimenti" PRIMARY KEY ("id")
);
CREATE TABLE "dw_hospital"."dim_periodi" (
	"id"                      INTEGER       NOT NULL,
	"language_code"           VARCHAR(2)    NOT NULL,
	"country_code"            VARCHAR(2)    NOT NULL,
	"month_abbreviation"      VARCHAR(3)    NOT NULL,
	"month_name"              VARCHAR(12)   NOT NULL,
	"quarter_name"            VARCHAR(8)    NOT NULL,
	"year_quarter"            VARCHAR(13)   NOT NULL,
	"year_month_abbreviation" VARCHAR(8)    NOT NULL,
	"year2"                   VARCHAR(2)    NOT NULL,
	"year4"                   INTEGER       NOT NULL,
	"month_number"            INTEGER       NOT NULL,
	"quarter_number"          INTEGER       NOT NULL,
	"year_month_number"       VARCHAR(8)    NOT NULL,
	"last_batch_id"           INTEGER       NOT NULL DEFAULT '0',
	"last_updated"            TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_dim_periodi" PRIMARY KEY ("id"),
	CONSTRAINT "IDX_dim_periodi" UNIQUE ("year4", "month_number")
);
create view "dw_hospital"."v_dim_classi_movimenti" as
 select 
 "dim_classi_movimenti"."id" as "id",
 "dim_classi_movimenti"."codice" as "codice",
 "dim_classi_movimenti"."descrizione" as "descrizione",
 "dim_classi_movimenti"."codice" || ' - ' || "dim_classi_movimenti"."descrizione" as "codice_descrizione",
 "dim_classi_movimenti"."tipo_centro" as "tipo_centro",
 "dim_classi_movimenti"."tipo_movimentazione" as "tipo_movimentazione",
 "dim_classi_movimenti"."tipo_caricamento" as "tipo_caricamento",
 "dim_classi_movimenti"."uso_ribaltamento" as "uso_ribaltamento",
 "dim_classi_movimenti"."version" as "version",
 "dim_classi_movimenti"."valid_from" as "valid_from",
 "dim_classi_movimenti"."valid_to" as "valid_to",
 "dim_classi_movimenti"."last_batch_id" as "last_batch_id",
 "dim_classi_movimenti"."last_updated" as "last_updated"
 from
 "dw_hospital"."dim_classi_movimenti"
;
CREATE TABLE "dw_hospital"."facts_costi_2017" (
	"id"                     INTEGER       NOT NULL DEFAULT next value for "dw_hospital"."seq_36397",
	"id_movimento_aggregato" INTEGER       NOT NULL,
	"periodo_id"             INTEGER       NOT NULL,
	"centro_id"              INTEGER       NOT NULL,
	"tipo_centro_id"         INTEGER       NOT NULL,
	"area_id"                INTEGER       NOT NULL,
	"fattore_id"             INTEGER       NOT NULL,
	"gestione_fattore_id"    INTEGER       NOT NULL,
	"tipo_fattore_id"        INTEGER       NOT NULL,
	"classe_movimento_id"    INTEGER       NOT NULL,
	"applicativo_id"         INTEGER       NOT NULL,
	"progetto_id"            INTEGER       NOT NULL,
	"valore"                 DECIMAL(19,4) NOT NULL,
	"quantita"               DECIMAL(19,4) NOT NULL,
	"last_batch_id"          INTEGER       NOT NULL,
	"archive"                INTEGER       NOT NULL,
	"last_updated"           TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_facts_costi_2017" PRIMARY KEY ("id")
);
CREATE TABLE "dw_hospital"."facts_costi_2018" (
	"id"                     INTEGER       NOT NULL DEFAULT next value for "dw_hospital"."seq_36422",
	"id_movimento_aggregato" INTEGER       NOT NULL,
	"periodo_id"             INTEGER       NOT NULL,
	"centro_id"              INTEGER       NOT NULL,
	"tipo_centro_id"         INTEGER       NOT NULL,
	"area_id"                INTEGER       NOT NULL,
	"fattore_id"             INTEGER       NOT NULL,
	"gestione_fattore_id"    INTEGER       NOT NULL,
	"tipo_fattore_id"        INTEGER       NOT NULL,
	"classe_movimento_id"    INTEGER       NOT NULL,
	"applicativo_id"         INTEGER       NOT NULL,
	"progetto_id"            INTEGER       NOT NULL,
	"valore"                 DECIMAL(19,4) NOT NULL,
	"quantita"               DECIMAL(19,4) NOT NULL,
	"last_batch_id"          INTEGER       NOT NULL,
	"archive"                INTEGER       NOT NULL,
	"last_updated"           TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_facts_costi_2018" PRIMARY KEY ("id")
);
CREATE TABLE "dw_hospital"."facts_costi_2019" (
	"id"                     INTEGER       NOT NULL DEFAULT next value for "dw_hospital"."seq_45407",
	"id_movimento_aggregato" INTEGER       NOT NULL,
	"periodo_id"             INTEGER       NOT NULL,
	"centro_id"              INTEGER       NOT NULL,
	"tipo_centro_id"         INTEGER       NOT NULL,
	"area_id"                INTEGER       NOT NULL,
	"fattore_id"             INTEGER       NOT NULL,
	"gestione_fattore_id"    INTEGER       NOT NULL,
	"tipo_fattore_id"        INTEGER       NOT NULL,
	"classe_movimento_id"    INTEGER       NOT NULL,
	"applicativo_id"         INTEGER       NOT NULL,
	"progetto_id"            INTEGER       NOT NULL,
	"valore"                 DECIMAL(19,4) NOT NULL,
	"quantita"               DECIMAL(19,4) NOT NULL,
	"last_batch_id"          INTEGER       NOT NULL,
	"archive"                INTEGER       NOT NULL,
	"last_updated"           TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_facts_costi_2019" PRIMARY KEY ("id")
);
CREATE MERGE TABLE "dw_hospital"."facts_costi" (
	"id"                     INTEGER       NOT NULL DEFAULT next value for "dw_hospital"."seq_45337",
	"id_movimento_aggregato" INTEGER       NOT NULL,
	"periodo_id"             INTEGER       NOT NULL,
	"centro_id"              INTEGER       NOT NULL,
	"tipo_centro_id"         INTEGER       NOT NULL,
	"area_id"                INTEGER       NOT NULL,
	"fattore_id"             INTEGER       NOT NULL,
	"gestione_fattore_id"    INTEGER       NOT NULL,
	"tipo_fattore_id"        INTEGER       NOT NULL,
	"classe_movimento_id"    INTEGER       NOT NULL,
	"applicativo_id"         INTEGER       NOT NULL,
	"progetto_id"            INTEGER       NOT NULL,
	"valore"                 DECIMAL(19,4) NOT NULL,
	"quantita"               DECIMAL(19,4) NOT NULL,
	"last_batch_id"          INTEGER       NOT NULL,
	"archive"                INTEGER       NOT NULL,
	"last_updated"           TIMESTAMP     NOT NULL DEFAULT current_timestamp(),
	CONSTRAINT "PK_facts_costi" PRIMARY KEY ("id")
);
ALTER TABLE "dw_hospital"."facts_costi" ADD TABLE "dw_hospital"."facts_costi_2017";
ALTER TABLE "dw_hospital"."facts_costi" ADD TABLE "dw_hospital"."facts_costi_2018";
ALTER TABLE "dw_hospital"."facts_costi" ADD TABLE "dw_hospital"."facts_costi_2019";
SET SCHEMA "sys";

INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (1,7,3829,'2019-07-18 16:05:31.0');
INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (4,7,3829,'2019-07-18 16:05:31.0');
INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (7,7,3829,'2019-07-18 16:05:31.0');
INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (20,7,3829,'2019-07-18 16:05:31.0');
INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (21,7,3829,'2019-07-18 16:05:31.0');
INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (24,7,3829,'2019-07-18 16:05:31.0');
INSERT INTO "dw_hospital"."bri_classi_gruppi_movimenti" (classe_movimento_id,gruppo_classe_movimento_id,last_batch_id,last_updated) VALUES (25,7,3829,'2019-07-18 16:05:31.0');

INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (0,'ND','Non Disponibile','Non Disponibile','Non Disponibile','Non Disponibile','Non Disponibile',1,'1900-01-01','2199-12-31',840,'2019-07-18 09:48:06.0');
INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (1,'1','Movimenti da Contabilita','Centro','Multi-periodo','Automatico','No',1,'1900-01-01','2200-01-01',842,'2019-07-18 09:48:06.0');
INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (4,'11','Competenza pers l.g.','Centro','Periodo Singolo','Automatico','No',1,'1900-01-01','2200-01-01',842,'2019-07-18 09:48:06.0');
INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (7,'1103','Consuntivo con budget','Gruppo','Non Definito','Non Definito','Si',1,'1900-01-01','2200-01-01',842,'2019-07-18 09:48:06.0');
INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (20,'2','Movimenti da Magazzino','Centro','Multi-periodo','Automatico','No',1,'1900-01-01','2200-01-01',842,'2019-07-18 09:48:06.0');
INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (21,'20','Attivita','Centro','Periodo Singolo','Automatico','No',1,'1900-01-01','2200-01-01',842,'2019-07-18 09:48:06.0');
INSERT INTO "dw_hospital"."dim_classi_movimenti" (id,codice,descrizione,tipo_centro,tipo_movimentazione,tipo_caricamento,uso_ribaltamento,version,valid_from,valid_to,last_batch_id,last_updated) VALUES (24,'38','Ribaltamenti utenze','Centro','Periodo Singolo','Automatico','Si',1,'1900-01-01','2200-01-01',842,'2019-07-18 09:48:06.0');

INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (1,'it','IT','ND','Non definita','ND','ND','ND','ND',0,0,0,'ND',0,'2012-05-25 16:11:06.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201701,'it','IT','gen','gennaio','I Trim','2017-I Trim','2017-gen','17',2017,1,1,'2017-01',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201702,'it','IT','feb','febbraio','I Trim','2017-I Trim','2017-feb','17',2017,2,1,'2017-02',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201703,'it','IT','mar','marzo','I Trim','2017-I Trim','2017-mar','17',2017,3,1,'2017-03',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201704,'it','IT','apr','aprile','II Trim','2017-II Trim','2017-apr','17',2017,4,2,'2017-04',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201705,'it','IT','mag','maggio','II Trim','2017-II Trim','2017-mag','17',2017,5,2,'2017-05',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201706,'it','IT','giu','giugno','II Trim','2017-II Trim','2017-giu','17',2017,6,2,'2017-06',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201707,'it','IT','lug','luglio','III Trim','2017-III Trim','2017-lug','17',2017,7,3,'2017-07',110,'2013-02-14 12:31:23.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201708,'it','IT','ago','agosto','III Trim','2017-III Trim','2017-ago','17',2017,8,3,'2017-08',110,'2013-02-14 12:31:23.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201709,'it','IT','set','settembre','III Trim','2017-III Trim','2017-set','17',2017,9,3,'2017-09',110,'2013-02-14 12:31:24.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201710,'it','IT','ott','ottobre','IV Trim','2017-IV Trim','2017-ott','17',2017,10,4,'2017-10',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201711,'it','IT','nov','novembre','IV Trim','2017-IV Trim','2017-nov','17',2017,11,4,'2017-11',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201712,'it','IT','dic','dicembre','IV Trim','2017-IV Trim','2017-dic','17',2017,12,4,'2017-12',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201801,'it','IT','gen','gennaio','I Trim','2018-I Trim','2018-gen','18',2018,1,1,'2018-01',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201802,'it','IT','feb','febbraio','I Trim','2018-I Trim','2018-feb','18',2018,2,1,'2018-02',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201803,'it','IT','mar','marzo','I Trim','2018-I Trim','2018-mar','18',2018,3,1,'2018-03',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201804,'it','IT','apr','aprile','II Trim','2018-II Trim','2018-apr','18',2018,4,2,'2018-04',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201805,'it','IT','mag','maggio','II Trim','2018-II Trim','2018-mag','18',2018,5,2,'2018-05',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201806,'it','IT','giu','giugno','II Trim','2018-II Trim','2018-giu','18',2018,6,2,'2018-06',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201807,'it','IT','lug','luglio','III Trim','2018-III Trim','2018-lug','18',2018,7,3,'2018-07',110,'2013-02-14 12:31:23.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201808,'it','IT','ago','agosto','III Trim','2018-III Trim','2018-ago','18',2018,8,3,'2018-08',110,'2013-02-14 12:31:23.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201809,'it','IT','set','settembre','III Trim','2018-III Trim','2018-set','18',2018,9,3,'2018-09',110,'2013-02-14 12:31:24.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201810,'it','IT','ott','ottobre','IV Trim','2018-IV Trim','2018-ott','18',2018,10,4,'2018-10',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201811,'it','IT','nov','novembre','IV Trim','2018-IV Trim','2018-nov','18',2018,11,4,'2018-11',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201812,'it','IT','dic','dicembre','IV Trim','2018-IV Trim','2018-dic','18',2018,12,4,'2018-12',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201901,'it','IT','gen','gennaio','I Trim','2019-I Trim','2019-gen','19',2019,1,1,'2019-01',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201902,'it','IT','feb','febbraio','I Trim','2019-I Trim','2019-feb','19',2019,2,1,'2019-02',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201903,'it','IT','mar','marzo','I Trim','2019-I Trim','2019-mar','19',2019,3,1,'2019-03',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201904,'it','IT','apr','aprile','II Trim','2019-II Trim','2019-apr','19',2019,4,2,'2019-04',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201905,'it','IT','mag','maggio','II Trim','2019-II Trim','2019-mag','19',2019,5,2,'2019-05',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201906,'it','IT','giu','giugno','II Trim','2019-II Trim','2019-giu','19',2019,6,2,'2019-06',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201907,'it','IT','lug','luglio','III Trim','2019-III Trim','2019-lug','19',2019,7,3,'2019-07',110,'2013-02-14 12:31:23.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201908,'it','IT','ago','agosto','III Trim','2019-III Trim','2019-ago','19',2019,8,3,'2019-08',110,'2013-02-14 12:31:23.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201909,'it','IT','set','settembre','III Trim','2019-III Trim','2019-set','19',2019,9,3,'2019-09',110,'2013-02-14 12:31:24.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201910,'it','IT','ott','ottobre','IV Trim','2019-IV Trim','2019-ott','19',2019,10,4,'2019-10',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201911,'it','IT','nov','novembre','IV Trim','2019-IV Trim','2019-nov','19',2019,11,4,'2019-11',110,'2012-05-25 16:13:07.0');
INSERT INTO "dw_hospital"."dim_periodi" (id,language_code,country_code,month_abbreviation,month_name,quarter_name,year_quarter,year_month_abbreviation,year2,year4,month_number,quarter_number,year_month_number,last_batch_id,last_updated) VALUES (201912,'it','IT','dic','dicembre','IV Trim','2019-IV Trim','2019-dic','19',2019,12,4,'2019-12',110,'2012-05-25 16:13:07.0');

INSERT INTO "dw_hospital"."facts_costi_2017" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (94606571,566810303,201701,455,4,23,2286,1,1,1,2,0,-25042.4700,-6.7900,3260,0,'2019-04-05 13:22:32.0');
INSERT INTO "dw_hospital"."facts_costi_2017" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (94606572,566810304,201701,1081,16,24,2279,1,1,4,2,0,18356.4500,2.1000,3260,0,'2019-04-05 13:22:32.0');
INSERT INTO "dw_hospital"."facts_costi_2017" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (94606573,566810305,201701,395,4,23,2281,1,1,20,2,0,25042.4700,6.7900,3260,0,'2019-04-05 13:22:32.0');
INSERT INTO "dw_hospital"."facts_costi_2017" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (94606574,566810306,201701,203,4,23,2283,1,1,21,2,0,14161.0700,4.6300,3260,0,'2019-04-05 13:22:32.0');
INSERT INTO "dw_hospital"."facts_costi_2017" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (94606575,566810307,201701,115,4,23,2286,1,1,24,2,0,-26517.5800,-6.8200,3260,0,'2019-04-05 13:22:32.0');

INSERT INTO "dw_hospital"."facts_costi_2018" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (95196386,731174788,201801,159,4,23,96,1,1,1,1,0,6912.4300,0.0000,3262,0,'2019-04-05 13:28:11.0');
INSERT INTO "dw_hospital"."facts_costi_2018" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (95196387,731174789,201801,6,0,23,98,1,1,4,1,0,-1097.2900,0.0000,3262,0,'2019-04-05 13:28:11.0');
INSERT INTO "dw_hospital"."facts_costi_2018" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (95196388,731174790,201801,212,3,23,98,1,1,20,1,0,432.0000,0.0000,3262,0,'2019-04-05 13:28:11.0');
INSERT INTO "dw_hospital"."facts_costi_2018" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (95196389,731174791,201801,525,4,23,101,1,1,21,1,0,7680.9500,0.0000,3262,0,'2019-04-05 13:28:11.0');
INSERT INTO "dw_hospital"."facts_costi_2018" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (95196390,731174792,201801,6,0,23,106,2,1,24,1,0,-30000.0000,-10.0000,3262,0,'2019-04-05 13:28:11.0');

INSERT INTO "dw_hospital"."facts_costi_2019" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (97251664,766351485,201901,291,4,23,2273,1,1,1,2,0,279.8800,0.0000,3822,0,'2019-07-17 13:24:49.0');
INSERT INTO "dw_hospital"."facts_costi_2019" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (97251665,766351486,201901,292,4,23,2273,1,1,20,2,0,3017.1400,0.0000,3822,0,'2019-07-17 13:24:49.0');
INSERT INTO "dw_hospital"."facts_costi_2019" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (97251666,766351487,201901,328,13,23,2273,1,1,4,2,0,2321.5700,0.0000,3822,0,'2019-07-17 13:24:49.0');
INSERT INTO "dw_hospital"."facts_costi_2019" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (97251667,766351488,201901,347,4,23,2273,1,1,24,2,0,192.9300,0.0000,3822,0,'2019-07-17 13:24:49.0');
INSERT INTO "dw_hospital"."facts_costi_2019" (id,id_movimento_aggregato,periodo_id,centro_id,tipo_centro_id,area_id,fattore_id,gestione_fattore_id,tipo_fattore_id,classe_movimento_id,applicativo_id,progetto_id,valore,quantita,last_batch_id,archive,last_updated) VALUES (97251668,766351489,201901,369,13,23,2273,1,1,21,2,0,2.3400,0.0000,3822,0,'2019-07-17 13:24:49.0');

select
"dim_periodi"."year4" as "c0",
"gruppi"."codice" as "c1",
count(*) as "m0"
from "dw_hospital"."dim_periodi" as "dim_periodi",
"dw_hospital"."facts_costi" as "facts_costi",
"dw_hospital"."v_dim_classi_movimenti" as "gruppi",
"dw_hospital"."bri_classi_gruppi_movimenti" as "bri_classi_gruppi_movimenti",
"dw_hospital"."v_dim_classi_movimenti" as "classi"
where "facts_costi"."periodo_id" = "dim_periodi"."id"
and "facts_costi"."classe_movimento_id" = "classi"."id"
and "classi"."id" = "bri_classi_gruppi_movimenti"."classe_movimento_id"
and "bri_classi_gruppi_movimenti"."gruppo_classe_movimento_id" = "gruppi"."id"
and "gruppi"."codice" = '1103'
group by "dim_periodi"."year4","gruppi"."codice"
;

rollback;
