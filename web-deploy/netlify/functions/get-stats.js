// ===================================================================
// Netlify Function: get-stats
// ===================================================================
// Purpose: Retrieve aggregated statistics for dashboard display
// Endpoint: GET /.netlify/functions/get-stats?period={today|7days|30days}
// ===================================================================

import { query } from './db-pool.js';

/**
 * Netlify serverless function handler
 * 
 * Query parameters:
 * - period: "today" | "7days" | "30days" (default: "7days")
 * 
 * Returns:
 * {
 *   "pump_on_count": 15,
 *   "pump_off_count": 15,
 *   "mode_changes": 3,
 *   "avg_rain_rate": 35.8,
 *   "avg_soil_moisture": 42.5,
 *   "last_updated": "2025-12-16T20:47:00Z",
 *   "period": "7days"
 * }
 */
export async function handler(event, context) {
    // CORS headers
    const headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Headers': 'Content-Type',
        'Access-Control-Allow-Methods': 'GET, OPTIONS',
        'Content-Type': 'application/json'
    };

    // Handle OPTIONS preflight
    if (event.httpMethod === 'OPTIONS') {
        return {
            statusCode: 204,
            headers,
            body: ''
        };
    }

    // Only accept GET
    if (event.httpMethod !== 'GET') {
        return {
            statusCode: 405,
            headers,
            body: JSON.stringify({ error: 'Method not allowed. Use GET.' })
        };
    }

    try {
        // Parse query parameters
        const period = event.queryStringParameters?.period || '7days';

        // Determine date range
        let intervalClause;
        switch (period) {
            case 'today':
                intervalClause = "timestamp >= CURRENT_DATE";
                break;
            case '30days':
                intervalClause = "timestamp >= NOW() - INTERVAL '30 days'";
                break;
            case '7days':
            default:
                intervalClause = "timestamp >= NOW() - INTERVAL '7 days'";
        }

        console.log('📊 Fetching stats for period:', period);

        // Query 1: Event counts
        const eventStats = await query(
            `SELECT 
         event_type,
         COUNT(*) as count
       FROM pump_events
       WHERE ${intervalClause}
       GROUP BY event_type`,
            []
        );

        // Query 2: Average soil moisture from sensor logs
        const moistureStats = await query(
            `SELECT 
         ROUND(AVG(soil_moisture), 2) as avg_moisture
       FROM sensor_logs
       WHERE ${intervalClause}`,
            []
        );

        // Query 3: Average rain rate from sensor logs
        const rainRateStats = await query(
            `SELECT 
         ROUND(AVG(rain_status), 2) as avg_rain_rate
       FROM sensor_logs
       WHERE ${intervalClause}`,
            []
        );

        // Extract counts
        const pumpOnCount = eventStats.rows.find(r => r.event_type === 'PUMP_ON')?.count || 0;
        const pumpOffCount = eventStats.rows.find(r => r.event_type === 'PUMP_OFF')?.count || 0;
        const modeChanges = eventStats.rows.find(r => r.event_type === 'MODE_CHANGE')?.count || 0;
        const avgMoisture = moistureStats.rows[0]?.avg_moisture || 0;
        const avgRainRate = rainRateStats.rows[0]?.avg_rain_rate || 0;

        console.log('✅ Stats retrieved:', {
            pump_on: pumpOnCount,
            pump_off: pumpOffCount,
            mode_changes: modeChanges,
            avg_rain_rate: avgRainRate
        });

        // Return aggregated stats
        return {
            statusCode: 200,
            headers,
            body: JSON.stringify({
                success: true,
                pump_on_count: parseInt(pumpOnCount) || 0,
                pump_off_count: parseInt(pumpOffCount) || 0,
                mode_changes: parseInt(modeChanges) || 0,
                avg_rain_rate: parseFloat(avgRainRate) || 0,
                avg_soil_moisture: parseFloat(avgMoisture) || 0,
                last_updated: new Date().toISOString(),
                period: period
            })
        };

    } catch (error) {
        console.error('❌ Error fetching stats:', error);

        return {
            statusCode: 500,
            headers,
            body: JSON.stringify({
                success: false,
                error: 'Failed to fetch statistics',
                details: error.message
            })
        };
    }
}
