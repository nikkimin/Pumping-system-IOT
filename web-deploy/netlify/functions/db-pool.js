// ===================================================================
// Database Connection Pool for Netlify Functions
// ===================================================================
// Purpose: Reusable connection pool to avoid creating new connections
//          for every function invocation
// ===================================================================

import pg from 'pg';

const { Pool } = pg;

// Create a singleton pool instance
let pool = null;

/**
 * Get or create PostgreSQL connection pool
 * Uses DATABASE_URL from Netlify environment variables
 * 
 * @returns {Pool} PostgreSQL connection pool
 */
export function getPool() {
  if (!pool) {
    const connectionString = process.env.DATABASE_URL;
    
    if (!connectionString) {
      throw new Error('DATABASE_URL environment variable is not set');
    }

    pool = new Pool({
      connectionString,
      ssl: {
        rejectUnauthorized: false // NeonDB requires SSL
      },
      max: 10, // Maximum number of connections in pool
      idleTimeoutMillis: 30000, // Close idle connections after 30s
      connectionTimeoutMillis: 10000, // Timeout after 10s if can't connect
    });

    // Log successful pool creation
    console.log('✅ Database pool created');

    // Handle pool errors
    pool.on('error', (err) => {
      console.error('❌ Unexpected database pool error:', err);
    });
  }

  return pool;
}

/**
 * Execute a parameterized query
 * 
 * @param {string} text - SQL query with $1, $2, etc. placeholders
 * @param {Array} params - Array of parameter values
 * @returns {Promise<Object>} Query result
 */
export async function query(text, params) {
  const pool = getPool();
  const start = Date.now();
  
  try {
    const result = await pool.query(text, params);
    const duration = Date.now() - start;
    
    console.log('✅ Query executed', { 
      duration: `${duration}ms`, 
      rows: result.rowCount 
    });
    
    return result;
  } catch (error) {
    console.error('❌ Database query error:', error.message);
    throw error;
  }
}

/**
 * Close the connection pool (useful for cleanup in tests)
 * Not typically needed in serverless functions
 */
export async function closePool() {
  if (pool) {
    await pool.end();
    pool = null;
    console.log('🔒 Database pool closed');
  }
}
