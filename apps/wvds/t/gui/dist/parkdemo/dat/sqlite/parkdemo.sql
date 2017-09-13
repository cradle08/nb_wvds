PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE "nodeattribution" (
"nodeid"  INTEGER NOT NULL,
"nodeip"  INTEGER,
"time"  TEXT NOT NULL,
"state"  INTEGER NOT NULL,
"route"  INTEGER,
"parkinglot"  TEXT
);
CREATE TABLE "samples" (
"nodeid"  INTEGER NOT NULL,
"time"  TEXT NOT NULL,
"parent"  INTEGER,
"seqno"  INTEGER,
"coordX"  REAL,
"coordY"  REAL,
"coordZ"  REAL,
"coordDesc"  TEXT,
"selfAddr"  INTEGER,
"isacar"  INTEGER,
"state"  INTEGER,
"voltage"  REAL,
"parkName"  TEXT
);
CREATE TABLE "blacklist" (
"mac"  TEXT NOT NULL,
"remark"  TEXT,
PRIMARY KEY ("mac")
);
CREATE TABLE "subnets" (
"id"  INTEGER NOT NULL,
"name"  TEXT NOT NULL,
"mapfile"  TEXT,
"remark"  TEXT,
PRIMARY KEY ("id" ASC)
);
INSERT INTO "subnets" VALUES(0,'Network',NULL,NULL);
INSERT INTO "subnets" VALUES(1,'Default','dat/map/blank.png',NULL);
INSERT INTO "subnets" VALUES(2,'凯达尔','dat/map/cadre.png',NULL);
INSERT INTO "subnets" VALUES(3,'石厦街','dat/map/shixia.png',NULL);
CREATE TABLE "views" (
"subnet_id"  INTEGER NOT NULL,
"xpos"  INTEGER NOT NULL,
"ypos"  INTEGER NOT NULL,
"scale"  REAL NOT NULL
);
INSERT INTO "views" VALUES(1,0,0,1.0);
INSERT INTO "views" VALUES(2,0,0,1.0);
INSERT INTO "views" VALUES(3,336,71,1.0);
INSERT INTO "views" VALUES(4,0,0,1.0);
CREATE TABLE "parkevts" (
"parklot"  TEXT NOT NULL,
"mac"  TEXT NOT NULL,
"state"  INTEGER NOT NULL,
"etime"  INTEGER NOT NULL DEFAULT 0,
"rtime"  INTEGER NOT NULL DEFAULT 0,
"seqno"  INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE "magnetic" (
"rtime"  INTEGER NOT NULL,
"mac"  TEXT(17) NOT NULL,
"magx"  REAL,
"magy"  REAL,
"magz"  REAL,
PRIMARY KEY ("rtime" ASC)
);
CREATE TABLE "motes" (
"id"  INTEGER NOT NULL,
"mac"  TEXT NOT NULL,
"panid"  INTEGER NOT NULL DEFAULT 1,
"name"  TEXT NOT NULL DEFAULT NA,
"type"  TEXT NOT NULL DEFAULT NA,
"hwver"  INTEGER NOT NULL DEFAULT 0,
"fwver"  INTEGER NOT NULL DEFAULT 0,
"x"  REAL NOT NULL DEFAULT 10,
"y"  REAL NOT NULL DEFAULT 10,
"fromIP"  TEXT,
"remark"  TEXT,
PRIMARY KEY ("mac" ASC)
);
INSERT INTO "motes" VALUES(416100001,'000004CA16100001',1,'Test','AP',16,16,5.898438,8.300781,'0.0.0.16',NULL);
INSERT INTO "motes" VALUES(416110002,'000004CA16110002',2,'Cadre','AP',16,16,9.549492,24.902216,'',NULL);
INSERT INTO "motes" VALUES(116110020,'000001CA16110020',2,'105901','VD',0,0,10.755076,17.470665,'',NULL);
INSERT INTO "motes" VALUES(116120002,'000001CA16120002',2,'105902','VD',0,0,9.359137,17.405476,'',NULL);
INSERT INTO "motes" VALUES(116110023,'000001CA16110023',2,'105903','VD',16,0,7.899746,17.405476,'',NULL);
INSERT INTO "motes" VALUES(116110030,'000001CA16110030',2,'105904','VD',16,0,6.472081,17.470665,'',NULL);
INSERT INTO "motes" VALUES(116110027,'000001CA16110027',2,'105905','VD',16,0,4.885787,17.405476,'',NULL);
INSERT INTO "motes" VALUES(416110004,'000004CA16110004',3,'Shixia','AP',16,16,23.975557,19.261745,'',NULL);
INSERT INTO "motes" VALUES(116120012,'000001CA16120012',3,'105183','VD',16,17,26.887132,32.483221,'',NULL);
INSERT INTO "motes" VALUES(116120023,'000001CA16120023',3,'105184','VD',0,0,26.851186,29.328859,'',NULL);
INSERT INTO "motes" VALUES(116120013,'000001CA16120013',3,'105185','VD',0,0,26.74335,26.375839,'',NULL);
INSERT INTO "motes" VALUES(116120006,'000001CA16120006',3,'105186','VD',0,0,26.671459,23.892617,'',NULL);
INSERT INTO "motes" VALUES(116120021,'000001CA16120021',3,'105187','VD',0,0,26.635514,21.073826,'',NULL);
INSERT INTO "motes" VALUES(116120015,'000001CA16120015',3,'105188','VD',0,0,26.563623,17.986577,'',NULL);
INSERT INTO "motes" VALUES(116120022,'000001CA16120022',3,'105190','VD',0,0,26.491733,14.09396,'',NULL);
INSERT INTO "motes" VALUES(116120004,'000001CA16120004',3,'105192','VD',0,0,26.455787,11.47651,'',NULL);
INSERT INTO "motes" VALUES(116120010,'000001CA16120010',3,'105193','VD',0,0,26.383896,8.791946,'',NULL);
INSERT INTO "motes" VALUES(116120011,'000001CA16120011',3,'105211','VD',0,0,27.965492,16.711409,'',NULL);
COMMIT;
