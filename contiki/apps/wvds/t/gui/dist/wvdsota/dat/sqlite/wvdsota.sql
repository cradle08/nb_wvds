PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE "motes" (
  "mac" TEXT NOT NULL,
  "type" INTEGER NOT NULL,
  "ver" INTEGER NOT NULL,
  "net_id" INTEGER NOT NULL,
  PRIMARY KEY ("mac")
);
CREATE TABLE "subnets" (
  "id" INTEGER NOT NULL,
  "city" TEXT NOT NULL,
  "district" TEXT NOT NULL,
  "street" TEXT NOT NULL,
  "rfchan" INTEGER NOT NULL,
  "rfpower" INTEGER NOT NULL,
  "vd_ver" INTEGER NOT NULL,
  "vd_file" TEXT NOT NULL,
  "vd_size" INTEGER NOT NULL,
  "rp_ver" INTEGER NOT NULL,
  "rp_file" TEXT NOT NULL,
  "rp_size" INTEGER NOT NULL,
  "ap_ver" INTEGER NOT NULL,
  "ap_file" TEXT NOT NULL,
  "ap_size" INTEGER NOT NULL,
  PRIMARY KEY ("id")
);
INSERT INTO "subnets" VALUES(1,'深圳市','南山区','默认网络',6,17,16,'VD.txt',0,16,'RP.txt',0,16,'AP.txt',0);
COMMIT;
