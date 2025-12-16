-- ===================================================================
-- NeonDB Schema for IoT Pumping System Statistics
-- ===================================================================
-- Created: 2025-12-16
-- Purpose: Track pump operations, sensor data, and generate statistics
-- ===================================================================

-- ===================================================================
-- TABLE 1: pump_events
-- Purpose: Record all pump and mode change events
-- ===================================================================
CREATE TABLE IF NOT EXISTS pump_events (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    event_type VARCHAR(20) NOT NULL,  -- 'PUMP_ON', 'PUMP_OFF', 'MODE_CHANGE'
    old_value VARCHAR(20),             -- Previous state: 'AUTO', 'MANUAL', 'ON', 'OFF'
    new_value VARCHAR(20),             -- New state: 'AUTO', 'MANUAL', 'ON', 'OFF'
    triggered_by VARCHAR(20),          -- 'manual', 'auto', 'mqtt'
    metadata JSONB,                    -- Additional data: {pump_speed: 50, soil_moisture: 30, ...}
    
    -- Constraints
    CONSTRAINT valid_event_type CHECK (event_type IN ('PUMP_ON', 'PUMP_OFF', 'MODE_CHANGE'))
);

-- Index for fast retrieval by timestamp (most recent first)
CREATE INDEX IF NOT EXISTS idx_pump_events_timestamp 
ON pump_events(timestamp DESC);

-- Index for filtering by event type
CREATE INDEX IF NOT EXISTS idx_pump_events_type 
ON pump_events(event_type);

-- Composite index for time-range queries with event type
CREATE INDEX IF NOT EXISTS idx_pump_events_time_type 
ON pump_events(timestamp DESC, event_type);

COMMENT ON TABLE pump_events IS 'Logs all pump state changes and mode switches for statistics and history tracking';

-- ===================================================================
-- TABLE 2: sensor_logs
-- Purpose: Store sensor readings over time
-- ===================================================================
CREATE TABLE IF NOT EXISTS sensor_logs (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    soil_moisture INT NOT NULL CHECK (soil_moisture >= 0 AND soil_moisture <= 100),
    rain_status BOOLEAN NOT NULL,
    pump_status BOOLEAN NOT NULL,
    auto_mode BOOLEAN NOT NULL,
    pump_speed INT CHECK (pump_speed >= 0 AND pump_speed <= 100)
);

-- Index for time-series queries
CREATE INDEX IF NOT EXISTS idx_sensor_logs_timestamp 
ON sensor_logs(timestamp DESC);

COMMENT ON TABLE sensor_logs IS 'Time-series sensor data for trend analysis and visualization';

-- ===================================================================
-- TABLE 3: daily_stats
-- Purpose: Pre-aggregated daily statistics for fast dashboard queries
-- ===================================================================
CREATE TABLE IF NOT EXISTS daily_stats (
    date DATE PRIMARY KEY,
    pump_on_count INT DEFAULT 0,
    pump_off_count INT DEFAULT 0,
    mode_changes INT DEFAULT 0,
    total_runtime_minutes INT DEFAULT 0,
    avg_soil_moisture DECIMAL(5,2),
    rain_hours INT DEFAULT 0,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

COMMENT ON TABLE daily_stats IS 'Aggregated daily statistics for performance optimization';

-- ===================================================================
-- VIEW: v_weekly_stats
-- Purpose: Quick access to last 7 days statistics
-- ===================================================================
CREATE OR REPLACE VIEW v_weekly_stats AS
SELECT 
    'PUMP_ON' as event_type,
    COUNT(*) as count,
    MAX(timestamp) as last_occurrence
FROM pump_events
WHERE timestamp >= NOW() - INTERVAL '7 days'
  AND event_type = 'PUMP_ON'
UNION ALL
SELECT 
    'PUMP_OFF' as event_type,
    COUNT(*) as count,
    MAX(timestamp) as last_occurrence
FROM pump_events
WHERE timestamp >= NOW() - INTERVAL '7 days'
  AND event_type = 'PUMP_OFF'
UNION ALL
SELECT 
    'MODE_CHANGE' as event_type,
    COUNT(*) as count,
    MAX(timestamp) as last_occurrence
FROM pump_events
WHERE timestamp >= NOW() - INTERVAL '7 days'
  AND event_type = 'MODE_CHANGE';

COMMENT ON VIEW v_weekly_stats IS 'Aggregated statistics for the last 7 days';

-- ===================================================================
-- FUNCTION: update_daily_stats()
-- Purpose: Trigger function to auto-update daily_stats table
-- ===================================================================
CREATE OR REPLACE FUNCTION update_daily_stats()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.event_type = 'PUMP_ON' THEN
        INSERT INTO daily_stats (date, pump_on_count)
        VALUES (CURRENT_DATE, 1)
        ON CONFLICT (date) 
        DO UPDATE SET 
            pump_on_count = daily_stats.pump_on_count + 1,
            updated_at = NOW();
    ELSIF NEW.event_type = 'PUMP_OFF' THEN
        INSERT INTO daily_stats (date, pump_off_count)
        VALUES (CURRENT_DATE, 1)
        ON CONFLICT (date) 
        DO UPDATE SET 
            pump_off_count = daily_stats.pump_off_count + 1,
            updated_at = NOW();
    ELSIF NEW.event_type = 'MODE_CHANGE' THEN
        INSERT INTO daily_stats (date, mode_changes)
        VALUES (CURRENT_DATE, 1)
        ON CONFLICT (date) 
        DO UPDATE SET 
            mode_changes = daily_stats.mode_changes + 1,
            updated_at = NOW();
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- ===================================================================
-- TRIGGER: trg_update_daily_stats
-- Purpose: Automatically update daily_stats on new pump_events
-- ===================================================================
CREATE TRIGGER trg_update_daily_stats
AFTER INSERT ON pump_events
FOR EACH ROW
EXECUTE FUNCTION update_daily_stats();

-- ===================================================================
-- SAMPLE DATA (Optional - for testing)
-- ===================================================================
-- Uncomment below to insert sample data for testing

-- INSERT INTO pump_events (event_type, old_value, new_value, triggered_by, metadata) VALUES
-- ('MODE_CHANGE', 'AUTO', 'MANUAL', 'mqtt', '{"soil_moisture": 45}'),
-- ('PUMP_ON', 'OFF', 'ON', 'manual', '{"pump_speed": 50, "soil_moisture": 30}'),
-- ('PUMP_OFF', 'ON', 'OFF', 'manual', '{"pump_speed": 50, "soil_moisture": 35}');

-- INSERT INTO sensor_logs (soil_moisture, rain_status, pump_status, auto_mode, pump_speed) VALUES
-- (45, false, false, true, 50),
-- (30, false, true, false, 50),
-- (35, false, false, false, 50);

-- ===================================================================
-- VERIFICATION QUERIES
-- ===================================================================
-- Run these after schema creation to verify setup

-- Check tables exist
-- SELECT table_name FROM information_schema.tables 
-- WHERE table_schema = 'public' 
-- ORDER BY table_name;

-- Check indexes
-- SELECT indexname FROM pg_indexes 
-- WHERE schemaname = 'public' 
-- ORDER BY indexname;

-- Verify view
-- SELECT * FROM v_weekly_stats;

-- ===================================================================
-- CLEANUP QUERIES (Use with caution!)
-- ===================================================================
-- Uncomment to reset database (WARNING: deletes all data)

-- DROP TRIGGER IF EXISTS trg_update_daily_stats ON pump_events;
-- DROP FUNCTION IF EXISTS update_daily_stats();
-- DROP VIEW IF EXISTS v_weekly_stats;
-- DROP TABLE IF EXISTS daily_stats CASCADE;
-- DROP TABLE IF EXISTS sensor_logs CASCADE;
-- DROP TABLE IF EXISTS pump_events CASCADE;
