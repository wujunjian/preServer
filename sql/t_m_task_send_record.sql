--DROP TABLE t_m_task_send_record;
create table t_m_task_send_record
(
cinema varchar2(8), 
payload varchar2(2),
status INTEGER,
send_time TIMESTAMP(6) default systimestamp
);

comment on column t_m_task_send_record.status is '0.�ɹ�, 1.δ����, 2.����ʧ��, 3.���ӱ��ر�';
