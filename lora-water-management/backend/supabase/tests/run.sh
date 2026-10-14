#!/usr/bin/env bash
# Şemayı ve RLS kurallarını geçici bir PostgreSQL veritabanında test eder.
# Gerekenler: psql ve erişilebilir bir PostgreSQL (PGHOST/PGUSER/PGPASSWORD ortam değişkenleri).
#   bash backend/supabase/tests/run.sh
set -euo pipefail
cd "$(dirname "$0")/.."
DB="${TEST_DB:-lwm_test}"
psql -v ON_ERROR_STOP=1 -q -d postgres -c "drop database if exists $DB" -c "create database $DB"
psql -v ON_ERROR_STOP=1 -q -d "$DB" -f tests/supabase_stub.sql
psql -v ON_ERROR_STOP=1 -q -d "$DB" -f migrations/20261014000000_init.sql
psql -v ON_ERROR_STOP=1 -q -d "$DB" -f tests/rls_test.sql
