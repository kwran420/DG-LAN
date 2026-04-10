/**
 * DG-LAN Website Helper
 * 
 * Fetches file data from your dglan-api server and builds dglan:// links.
 * 
 * Usage:
 *   const api = new DglanApi("https://your-server.com:8080");
 *   const data = await api.getFiles();
 *   // data.files[0].dglan_url → "dglan://download?peer=...&hash=...&size=...&name=..."
 */

class DglanApi {
    constructor(baseUrl) {
        this.baseUrl = baseUrl.replace(/\/$/, "");
    }

    /** Fetch all shared files with dglan:// URLs pre-built. */
    async getFiles() {
        const resp = await fetch(`${this.baseUrl}/api/v1/files`);
        if (!resp.ok) throw new Error(`API error: ${resp.status}`);
        return resp.json();
    }

    /** Check Core connection status and cache progress. */
    async getStatus() {
        const resp = await fetch(`${this.baseUrl}/api/v1/status`);
        if (!resp.ok) throw new Error(`API error: ${resp.status}`);
        return resp.json();
    }

    /** Health check — returns true if server is reachable. */
    async isHealthy() {
        try {
            const resp = await fetch(`${this.baseUrl}/api/v1/health`);
            return resp.ok;
        } catch {
            return false;
        }
    }

    /**
     * Build a dglan:// download link from file parameters.
     * Useful if you store the raw data and build links client-side.
     */
    static buildLink({ peer, hash, size, name, path }) {
        if (!peer || !hash || !name) {
            throw new Error("buildLink requires peer, hash, and name");
        }
        const params = new URLSearchParams({
            peer,
            hash,
            size: String(size),
            name,
            path: path || "/",
        });
        return `dglan://download?${params.toString()}`;
    }

    /** Format file size for display. */
    static formatSize(bytes) {
        if (!Number.isFinite(bytes) || bytes < 0) return "0 B";
        if (bytes === 0) return "0 B";
        const BYTES_PER_UNIT = 1024;
        const units = ["B", "KB", "MB", "GB", "TB"];
        const i = Math.floor(Math.log(bytes) / Math.log(BYTES_PER_UNIT));
        return `${(bytes / Math.pow(BYTES_PER_UNIT, i)).toFixed(i ? 1 : 0)} ${units[i]}`;
    }
}

if (typeof module !== "undefined") module.exports = DglanApi;
