// ===================================================================
// Netlify Function: log-sensor
// ===================================================================
// Purpose: Log sensor data to database for trend analysis
// Endpoint: POST /.netlify/functions/log-sensor
// Note: Called every 5 minutes to reduce database load
// ===================================================================

import { query } from './db-pool.js';

/**
 * Netlify serverless function handler
 * 
 * Expected request body:
 * {
 *   "soil_moisture": 45,        // 0-100%
 *   "rain_status": 75,          // 0-100% (rain probability)
 *   "pump_status": true,        // boolean
 *   "auto_mode": true,          // boolean
 *   "pump_speed": 50            // 0-100% (optional)
 * }
 */
export async function handler(event, context) {
    // CORS headers
    const headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Headers': 'Content-Type',
        'Access-Control-Allow-Methods': 'POST, OPTIONS',
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

    // Only accept POST
    if (event.httpMethod !== 'POST') {
        return {
            statusCode: 405,
            headers,
            body: JSON.stringify({ error: 'Method not allowed. Use POST.' })
        };
    }

    try {
        // Parse request body
        const {
            soil_moisture,
            rain_status,
            pump_status,
            auto_mode,
            pump_speed
        } = JSON.parse(event.body);

        // Validate required fields
        if (
            soil_moisture === undefined ||
            rain_status === undefined ||
            pump_status === undefined ||
            auto_mode === undefined
        ) {
            return {
                statusCode: 400,
                headers,
                body: JSON.stringify({
                    error: 'Missing required fields: soil_moisture, rain_status, pump_status, auto_mode'
                })
            };
        }

        // Validate ranges
        if (soil_moisture < 0 || soil_moisture > 100) {
            return {
                statusCode: 400,
                headers,
                body: JSON.stringify({
                    error: 'soil_moisture must be between 0 and 100'
                })
            };
        }

        // Validate rain_status range (0-100% probability)
        if (rain_status < 0 || rain_status > 100) {
            return {
                statusCode: 400,
                headers,
                body: JSON.stringify({
                    error: 'rain_status must be between 0 and 100'
                })
            };
        }

        if (pump_speed !== undefined && (pump_speed < 0 || pump_speed > 100)) {
            return {
                statusCode: 400,
                headers,
                body: JSON.stringify({
                    error: 'pump_speed must be between 0 and 100'
                })
            };
        }

        console.log('📊 Logging sensor data:', { soil_moisture, rain_status, pump_status, auto_mode, pump_speed });

        // Insert sensor data
        const result = await query(
            `INSERT INTO sensor_logs (soil_moisture, rain_status, pump_status, auto_mode, pump_speed)
       VALUES ($1, $2, $3, $4, $5)
       RETURNING id, timestamp`,
            [
                soil_moisture,
                rain_status,
                pump_status,
                auto_mode,
                pump_speed || null
            ]
        );

        const insertedLog = result.rows[0];

        console.log('✅ Sensor data logged:', insertedLog);

        // Return success
        return {
            statusCode: 200,
            headers,
            body: JSON.stringify({
                success: true,
                log_id: insertedLog.id,
                timestamp: insertedLog.timestamp,
                message: 'Sensor data logged successfully'
            })
        };

    } catch (error) {
        console.error('❌ Error logging sensor data:', error);

        return {
            statusCode: 500,
            headers,
            body: JSON.stringify({
                success: false,
                error: 'Failed to log sensor data',
                details: error.message
            })
        };
    }
}
