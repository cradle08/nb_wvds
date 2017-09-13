# --------------------------------------------------------
# Host:                         127.0.0.1
# Server version:               5.5.13
# Server OS:                    Win32
# HeidiSQL version:             6.0.0.3603
# Date/time:                    2014-06-11 20:53:35
# --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

# Dumping database structure for wsnits
DROP DATABASE IF EXISTS `wsnits`;
CREATE DATABASE IF NOT EXISTS `wsnits` /*!40100 DEFAULT CHARACTER SET latin1 */;
USE `wsnits`;


# Dumping structure for table wsnits.nodeattribution
DROP TABLE IF EXISTS `nodeattribution`;
CREATE TABLE IF NOT EXISTS `nodeattribution` (
  `id` int(10) NOT NULL AUTO_INCREMENT,
  `time` datetime DEFAULT NULL,
  `nodeid` int(10) DEFAULT NULL,
  `nodeip` int(10) DEFAULT NULL,
  `route` int(10) DEFAULT NULL,
  `parkinglot` varchar(10) DEFAULT NULL,
  `lamp` int(10) DEFAULT NULL,
  `state` int(10) DEFAULT NULL,
  KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

# Data exporting was unselected.


# Dumping structure for table wsnits.samples
DROP TABLE IF EXISTS `samples`;
CREATE TABLE IF NOT EXISTS `samples` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `time` datetime NOT NULL,
  `nodeid` int(10) NOT NULL,
  `selfAddr` int(10) DEFAULT NULL,
  `parent` int(10) DEFAULT NULL,
  `errno` int(11) DEFAULT NULL,
  `isacar` tinyint(1) DEFAULT NULL,
  `seqno` int(11) DEFAULT NULL,
  `parkName` varchar(50) DEFAULT NULL,
  `coordX` int(11) DEFAULT NULL,
  `coordY` int(11) DEFAULT NULL,
  `coordZ` int(11) DEFAULT NULL,
  `coordDesc` int(11) DEFAULT NULL,
  `state` int(11) DEFAULT NULL,
  `rssi` int(11) DEFAULT NULL,
  `loss` int(11) DEFAULT NULL,
  `carCount` int(10) DEFAULT NULL,
  `data` float DEFAULT NULL,
  `voltage` int(10) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Table to store sample datas';

# Data exporting was unselected.
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
