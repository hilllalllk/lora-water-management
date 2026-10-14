-- LoRa Su Yönetim Sistemi - ilk şema
-- Tasarım: docs/02-sistem-mimarisi.md §6
--
-- Kurulum: Supabase panelinde SQL Editor'e yapıştırıp çalıştırın
-- veya Supabase CLI ile: supabase db push

-- ---------------------------------------------------------------------------
-- Yardımcı: istek yapan kullanıcı gateway mi?
-- Gateway kullanıcısının app_metadata alanında {"role": "gateway"} bulunur.
-- app_metadata'yı sadece yönetici (service_role) değiştirebilir.
-- ---------------------------------------------------------------------------
create or replace function public.is_gateway() returns boolean
  language sql stable
as $$
  select coalesce((auth.jwt() -> 'app_metadata' ->> 'role') = 'gateway', false)
$$;

-- ---------------------------------------------------------------------------
-- Tablolar
-- ---------------------------------------------------------------------------

-- Cihazlar ve son durumları
create table public.devices (
  id            text primary key,                              -- ör. 'node-1001'
  address       integer not null unique
                check (address between 2 and 65534),           -- LoRa adresi (0x0001 gateway, 0xFFFF yayın)
  name          text not null,
  type          text not null default 'tank+valve',
  fw            text,
  period_s      integer not null default 60 check (period_s >= 10),
  level_mm      integer,
  valve         smallint check (valve between 0 and 3),        -- 0 kapalı, 1 açık, 2 hareket, 3 hata
  pressure_cbar integer,
  flow_dl_min   integer,
  alarms        smallint not null default 0,
  last_seen     timestamptz,
  created_at    timestamptz not null default now()
);
comment on table public.devices is 'Saha düğümleri ve son bilinen durumları';

-- Ölçüm geçmişi (her TELEMETRY / CMD_RESULT bir satır)
create table public.telemetry (
  id            bigint generated always as identity primary key,
  device_id     text not null references public.devices(id) on delete cascade,
  received_at   timestamptz not null default now(),
  level_mm      integer,
  valve         smallint,
  pressure_cbar integer,
  flow_dl_min   integer,
  alarms        smallint
);
create index telemetry_device_time_idx on public.telemetry (device_id, received_at desc);
comment on table public.telemetry is 'Gateway tarafından eklenen ölçüm geçmişi';

-- Komut kuyruğu
create type public.command_status as enum ('pending', 'sent', 'delivered', 'done', 'failed');

create table public.commands (
  id          bigint generated always as identity primary key,  -- LoRa'da cmdId = id & 0xFFFF
  device_id   text not null references public.devices(id) on delete cascade,
  op          text not null
              check (op in ('VANA_AC', 'VANA_KAPAT', 'CIHAZ_ETKIN', 'SIMDI_OKU', 'PERIYOT_AYARLA')),
  args        jsonb,
  status      public.command_status not null default 'pending',
  result      smallint,
  created_by  uuid default auth.uid() references auth.users(id) on delete set null,
  created_at  timestamptz not null default now(),
  updated_at  timestamptz not null default now()
);
create index commands_pending_idx on public.commands (id) where status = 'pending';
comment on table public.commands is 'Mobil uygulamadan sahaya giden komutlar';

-- Gateway sağlığı
create table public.gateways (
  id         text primary key,
  fw         text,
  wifi_rssi  integer,
  last_seen  timestamptz
);

-- ---------------------------------------------------------------------------
-- Tetikleyiciler
-- ---------------------------------------------------------------------------

-- Yeni telemetri gelince cihazın son durumunu güncelle
create or replace function public.apply_telemetry() returns trigger
  language plpgsql security definer set search_path = public
as $$
begin
  update public.devices set
    level_mm      = coalesce(new.level_mm, level_mm),
    valve         = coalesce(new.valve, valve),
    pressure_cbar = coalesce(new.pressure_cbar, pressure_cbar),
    flow_dl_min   = coalesce(new.flow_dl_min, flow_dl_min),
    alarms        = coalesce(new.alarms, alarms),
    last_seen     = new.received_at
  where id = new.device_id;
  return new;
end
$$;

create trigger telemetry_apply
  after insert on public.telemetry
  for each row execute function public.apply_telemetry();

-- commands.updated_at alanını otomatik güncelle
create or replace function public.touch_updated_at() returns trigger
  language plpgsql
as $$
begin
  new.updated_at = now();
  return new;
end
$$;

create trigger commands_touch
  before update on public.commands
  for each row execute function public.touch_updated_at();

-- PERIYOT_AYARLA başarıyla tamamlanınca devices.period_s'i güncelle
create or replace function public.apply_period_command() returns trigger
  language plpgsql security definer set search_path = public
as $$
begin
  if new.op = 'PERIYOT_AYARLA' and new.status = 'done' and old.status is distinct from 'done' then
    update public.devices
       set period_s = (new.args ->> 'seconds')::integer
     where id = new.device_id;
  end if;
  return new;
end
$$;

create trigger commands_apply_period
  after update on public.commands
  for each row execute function public.apply_period_command();

-- ---------------------------------------------------------------------------
-- Satır düzeyi güvenlik (RLS)
-- ---------------------------------------------------------------------------
alter table public.devices   enable row level security;
alter table public.telemetry enable row level security;
alter table public.commands  enable row level security;
alter table public.gateways  enable row level security;

-- Okuma: giriş yapmış herkes
create policy "devices okuma"   on public.devices   for select to authenticated using (true);
create policy "telemetry okuma" on public.telemetry for select to authenticated using (true);
create policy "commands okuma"  on public.commands  for select to authenticated using (true);
create policy "gateways okuma"  on public.gateways  for select to authenticated using (true);

-- devices: ekleme/silme sadece panelden (service_role). Gateway heartbeat için günceller.
create policy "devices gateway gunceller" on public.devices
  for update to authenticated using (public.is_gateway()) with check (public.is_gateway());

-- telemetry: sadece gateway ekler, kimse güncelleyemez
create policy "telemetry gateway ekler" on public.telemetry
  for insert to authenticated with check (public.is_gateway());

-- commands: kullanıcı sadece 'pending' komut ekler, durumunu sadece gateway değiştirir
create policy "commands kullanici ekler" on public.commands
  for insert to authenticated
  with check (status = 'pending' and result is null and created_by = auth.uid());
create policy "commands gateway gunceller" on public.commands
  for update to authenticated using (public.is_gateway()) with check (public.is_gateway());

-- gateways: sadece gateway kendi kaydını ekler/günceller
create policy "gateways gateway ekler" on public.gateways
  for insert to authenticated with check (public.is_gateway());
create policy "gateways gateway gunceller" on public.gateways
  for update to authenticated using (public.is_gateway()) with check (public.is_gateway());

-- ---------------------------------------------------------------------------
-- Realtime: mobil uygulama bu tablolardaki değişiklikleri canlı alır
-- ---------------------------------------------------------------------------
alter publication supabase_realtime add table public.devices, public.commands;

-- ---------------------------------------------------------------------------
-- Bakım: eski telemetriyi sil (ücretsiz planda 500 MB sınırı)
-- Örnek: select public.cleanup_old_telemetry(90);
-- ---------------------------------------------------------------------------
create or replace function public.cleanup_old_telemetry(keep_days integer default 90)
  returns bigint
  language sql security definer set search_path = public
as $$
  with deleted as (
    delete from public.telemetry
     where received_at < now() - make_interval(days => keep_days)
    returning 1
  )
  select count(*) from deleted
$$;
revoke execute on function public.cleanup_old_telemetry(integer) from public, anon, authenticated;
