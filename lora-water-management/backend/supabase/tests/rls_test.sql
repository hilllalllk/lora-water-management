-- Şema ve RLS kuralları testi. Çalıştırma: backend/supabase/tests/run.sh
\set ON_ERROR_STOP on
\set QUIET on

grant usage on schema public to authenticated;
grant select, insert, update on all tables in schema public to authenticated;

insert into auth.users values
  ('00000000-0000-0000-0000-00000000000a'),   -- normal kullanıcı
  ('00000000-0000-0000-0000-00000000000b');   -- gateway
\i seed.sql

create function pg_temp.as_user() returns void language sql as $$
  select set_config('request.jwt.claims',
    '{"sub":"00000000-0000-0000-0000-00000000000a","app_metadata":{}}', false);
$$;
create function pg_temp.as_gateway() returns void language sql as $$
  select set_config('request.jwt.claims',
    '{"sub":"00000000-0000-0000-0000-00000000000b","app_metadata":{"role":"gateway"}}', false);
$$;
create function pg_temp.expect_denied(sql text) returns void language plpgsql as $$
begin
  execute sql;
  raise exception 'BEKLENMEYEN: izin verildi -> %', sql;
exception when insufficient_privilege or check_violation then
  null;  -- beklenen
end $$;

set role authenticated;

-- 1) Kullanıcı komut ekleyebilir
do $t$ begin perform pg_temp.as_user(); end $t$;
insert into public.commands (device_id, op) values ('node-1001', 'VANA_AC');
\echo 'OK  kullanici pending komut ekler'

-- 2) Kullanıcı 'done' durumunda komut ekleyemez
do $t$ begin perform pg_temp.expect_denied($$insert into public.commands (device_id, op, status) values ('node-1001','VANA_AC','done')$$); end $t$;
\echo 'OK  kullanici done komut ekleyemez'

-- 3) Kullanıcı telemetri ekleyemez
do $t$ begin perform pg_temp.expect_denied($$insert into public.telemetry (device_id, level_mm) values ('node-1001', 5)$$); end $t$;
\echo 'OK  kullanici telemetri ekleyemez'

-- 4) Kullanıcı komut durumunu değiştiremez (RLS satırı gizler: 0 satır güncellenir)
do $$
declare n int;
begin
  update public.commands set status = 'done';
  get diagnostics n = row_count;
  if n <> 0 then raise exception 'kullanici komut guncelledi'; end if;
end $$;
\echo 'OK  kullanici komut durumunu degistiremez'

-- 5) Gateway telemetri ekler, tetikleyici devices satırını günceller
do $t$ begin perform pg_temp.as_gateway(); end $t$;
insert into public.telemetry (device_id, level_mm, valve) values ('node-1001', 1250, 1);
do $$
begin
  if (select level_mm from public.devices where id = 'node-1001') <> 1250 then
    raise exception 'tetikleyici devices.level_mm guncellemedi';
  end if;
  if (select last_seen from public.devices where id = 'node-1001') is null then
    raise exception 'last_seen bos';
  end if;
end $$;
\echo 'OK  gateway telemetri ekler, devices guncellenir'

-- 6) Gateway komut durumunu günceller, updated_at değişir
update public.commands set status = 'done', result = 0 where op = 'VANA_AC';
do $$
begin
  if (select count(*) from public.commands where status = 'done') <> 1 then
    raise exception 'gateway komutu guncelleyemedi';
  end if;
end $$;
\echo 'OK  gateway komut durumunu gunceller'

-- 7) PERIYOT_AYARLA tamamlanınca devices.period_s güncellenir
do $t$ begin perform pg_temp.as_user(); end $t$;
insert into public.commands (device_id, op, args) values ('node-1001', 'PERIYOT_AYARLA', '{"seconds": 30}');
do $t$ begin perform pg_temp.as_gateway(); end $t$;
update public.commands set status = 'done', result = 0 where op = 'PERIYOT_AYARLA';
do $$
begin
  if (select period_s from public.devices where id = 'node-1001') <> 30 then
    raise exception 'period_s guncellenmedi';
  end if;
end $$;
\echo 'OK  PERIYOT_AYARLA devices.period_s gunceller'

-- 8) Gateway "now" ile heartbeat yazar (firmware bu değeri gönderir)
insert into public.gateways (id, fw, last_seen) values ('gw-01', '0.1.0', 'now');
update public.devices set last_seen = 'now', fw = '0.1.0' where id = 'node-1002';
\echo 'OK  gateway heartbeat yazar'

-- 9) Giriş yapmamış (anon) kimse veri göremez. Supabase'de anon'un tablo yetkisi vardır,
--    ama ona açık bir RLS kuralı olmadığı için 0 satır döner.
reset role;
grant usage on schema public to anon;
grant select on all tables in schema public to anon;
set role anon;
do $$
begin
  if (select count(*) from public.devices) <> 0 then raise exception 'anon cihazlari gordu'; end if;
  if (select count(*) from public.commands) <> 0 then raise exception 'anon komutlari gordu'; end if;
end $$;
\echo 'OK  anon okuyamaz'

reset role;
\echo 'TUM TESTLER GECTI'
