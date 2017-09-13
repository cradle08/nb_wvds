delete from blacklist;
delete from magnetic;
delete from motes;
delete from nodeattribution;
delete from parkevts;
delete from samples;
delete from subnets where id>1;
delete from views;
vacuum;
