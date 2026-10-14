-- Örnek veriler (geliştirme için). Supabase panelinde SQL Editor'den çalıştırın.
insert into public.devices (id, address, name, type) values
  ('node-1001', 4097, 'Depo-1 (prototip)', 'tank+valve'),   -- 0x1001
  ('node-1002', 4098, 'Terfi-1 (prototip)', 'tank+valve')   -- 0x1002
on conflict (id) do nothing;
