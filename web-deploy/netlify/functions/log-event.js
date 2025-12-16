// ===================================================================
// Netlify Function: log-event
// ===================================================================
// Purpose: Log pump and mode change events to database
// Endpoint: POST /.netlify/functions/log-event
// ===================================================================

import { query } from './db-pool.js';

/**
 * Netlify serverless function handler
 * 
 * Expected request body:
 * {
 *   "event_type": "PUMP_ON" | "PUMP_OFF" | "MODE_CHANGE",
 *   "old_value": "AUTO" | "MANUAL" | "ON" | "OFF",
 *   "new_value": "AUTO" | "MANUAL" | "ON" | "OFF",
 *   "triggered_by": "manual" | "auto" | "mqtt",
 *   "metadata": { soil_moisture: 45, pump_speed: 50, ... }
 * }
 */
export async function handler(event, context) {
    // CORS headers for frontend access
    const headers = {
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Headers': 'Content-Type',
        'Access-Control-Allow-Methods': 'POST, OPTIONS',
        'Content-Type': 'application/json'
    };

    // Handle OPTIONS preflight request
    if (event.httpMethod === 'OPTIONS') {
        return {
            statusCode: 204,
            headers,
            body: ''
        };
    }

    // Only accept POST requests
    if (event.httpMethod !== 'POST') {
        return {
            statusCode: 405,
            headers,
            body: JSON.stringify({ error: 'Method not allowed. Use POST.' })
        };
    }

    try {
        // Parse request body
        const { event_type, old_value, new_value, triggered_by, metadata } = JSON.parse(event.body);

        // Validate required fields
        if (!event_type || !new_value) {
            return {
                statusCode: 400,
                headers,
                body: JSON.stringify({
                    error: 'Missing required fields: event_type, new_value'
                })
            };
        }

        // Validate event_type
        const validEventTypes = ['PUMP_ON', 'PUMP_OFF', 'MODE_CHANGE'];
        if (!validEventTypes.includes(event_type)) {
            return {
                statusCode: 400,
                headers,
                body: JSON.stringify({
                    error: `Invalid event_type. Must be one of: ${validEventTypes.join(', ')}`
                })
            };
        }

        console.log('📝 Logging event:', { event_type, old_value, new_value, triggered_by });

        // Insert event into database
        const result = await query(
            `INSERT INTO pump_events (event_type, old_value, new_value, triggered_by, metadata)
       VALUES ($1, $2, $3, $4, $5)
       RETURNING id, timestamp`,
            [
                event_type,
                old_value || null,
                new_value,
                triggered_by || 'mqtt',
                metadata ? JSON.stringify(metadata) : null
            ]
        );

        const insertedEvent = result.rows[0];

        console.log('✅ Event logged successfully:', insertedEvent);

        // Return success response
        return {
            statusCode: 200,
            headers,
            body: JSON.stringify({
                success: true,
                event_id: insertedEvent.id,
                timestamp: insertedEvent.timestamp,
                message: 'Event logged successfully'
            })
        };

    } catch (error) {
        console.error('❌ Error logging event:', error);

        // Return error response
        return {
            statusCode: 500,
            headers,
            body: JSON.stringify({
                success: false,
                error: 'Failed to log event',
                details: error.message
            })
        };
    }
}
