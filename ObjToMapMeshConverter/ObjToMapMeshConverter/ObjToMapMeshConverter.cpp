
// PIPELINE:
//  - Group triangles by shader
//  - For each shader group:
//      ALWAYS write 2x2 mesh patches (triangle soup)
//
// USER OPTION:
//  - Surface mode:
//      1 = Keep original shaders (requires MTL)
//      2 = Force CAULK on all surfaces (skip MTL prompt)
//
// OUTPUT OPTION:
//  - 1 = Single file
//  - 2 = Split into N files (2 to 10)
//
// SCALE OPTION:
//  - 1 = Divide by 2.54 (HUSKY export)
//  - 2 = Keep original
//  - 3 = Custom scale factor (multiplies output)

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <cstring>
#include <limits>
#include <windows.h>
#include <cfloat>

using namespace std;

struct TVector2 { float x = 0, y = 0; };
struct TVector3 { float x = 0, y = 0, z = 0; };

struct TSimple_Vertex
{
    TVector3 position;
    TVector2 uv;
    TVector3 normal;
};

struct Triangle
{
    size_t i0, i1, i2;
    string shader;
};

struct tGeometry
{
    vector<TSimple_Vertex> vertices;
    vector<Triangle> tris;
};

static int   scaleMode = 2;    // 1=/2.54, 2=keep, 3=custom
static float customScale = 1.0f; // used if scaleMode==3
static int   rotateXMode = 0;    // 0 none, 1 +90, 2 -90, 3 180

// ============================================================
// USER-SELECTED CAULK OVERRIDE
// ============================================================
static bool forceCaulkAll = false;

// ============================================================
// Robust input helpers
// ============================================================

static void clearLine()
{
    cin.clear();
    cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
}

static int askIntInRange(const string& prompt, int minV, int maxV)
{
    while (true)
    {
        cout << prompt;
        long long v;
        if (cin >> v)
        {
            clearLine();
            if (v >= minV && v <= maxV)
                return (int)v;
        }
        else
        {
            clearLine();
        }

        cout << "Invalid input. Please enter a number from " << minV << " to " << maxV << ".\n";
    }
}

static float askFloatInRange(const string& prompt, float minV, float maxV)
{
    while (true)
    {
        cout << prompt;
        double v;
        if (cin >> v)
        {
            clearLine();
            if (v >= (double)minV && v <= (double)maxV)
                return (float)v;
        }
        else
        {
            clearLine();
        }

        cout << "Invalid input. Please enter a number from " << minV << " to " << maxV << ".\n";
    }
}

// ============================================================
// Utility
// ============================================================

static string trim(const string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static string toLowerCopy(string s)
{
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

static bool endsWithNoCase(const string& s, const string& suffix)
{
    if (s.size() < suffix.size()) return false;
    string a = toLowerCopy(s.substr(s.size() - suffix.size()));
    string b = toLowerCopy(suffix);
    return a == b;
}

static bool fileExistsAndNotDir(const string& path)
{
    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return false;
    if (attr & FILE_ATTRIBUTE_DIRECTORY) return false;
    return true;
}

static string readDroppedPathLine()
{
    string s;
    getline(cin, s);
    s = trim(s);

    // Handle empty line (common if a leftover newline is consumed)
    while (s.empty() && !cin.fail())
    {
        // If user just hit enter, re-prompt via returning empty
        return "";
    }

    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        s = s.substr(1, s.size() - 2);

    return trim(s);
}

static string getStemNoFilesystem(const string& path)
{
    string name = path;
    size_t slash = name.find_last_of("\\/");
    if (slash != string::npos) name.erase(0, slash + 1);
    size_t dot = name.find_last_of(".");
    if (dot != string::npos) name.erase(dot);
    return name;
}

static bool replaceFileAtomicA(const string& dst, const string& src)
{
    return MoveFileExA(src.c_str(), dst.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

// Re-prompt until we get a valid dropped file of allowed extension(s)
static string promptForDroppedFile(const string& prompt, const vector<string>& allowedExts)
{
    while (true)
    {
        cout << prompt;
        string path = readDroppedPathLine();

        if (path.empty())
        {
            cout << "No path detected. Please drag-and-drop the file into this window and press Enter.\n";
            continue;
        }

        if (!fileExistsAndNotDir(path))
        {
            cout << "That file does not exist (or is a folder). Please drop a valid file.\n";
            continue;
        }

        bool okExt = false;
        for (const auto& ext : allowedExts)
        {
            if (endsWithNoCase(path, ext))
            {
                okExt = true;
                break;
            }
        }

        if (!okExt)
        {
            cout << "Unsupported file type. Expected: ";
            for (size_t i = 0; i < allowedExts.size(); ++i)
            {
                cout << allowedExts[i];
                if (i + 1 < allowedExts.size()) cout << ", ";
            }
            cout << "\n";
            continue;
        }

        return path;
    }
}

// ============================================================
// map_Kd path -> shader name
// ============================================================

static string shaderFromMapKd(string path)
{
    path = trim(path);
    for (char& c : path) if (c == '/') c = '\\';

    size_t p = path.find_last_of('\\');
    string file = (p == string::npos) ? path : path.substr(p + 1);

    size_t dot = file.find_last_of('.');
    string stem = (dot == string::npos) ? file : file.substr(0, dot);

    auto dropSuffix = [&](const char* suf)
        {
            size_t n = strlen(suf);
            if (stem.size() > n && stem.compare(stem.size() - n, n, suf) == 0)
                stem.erase(stem.size() - n);
        };

    dropSuffix("_col");
    dropSuffix("_diff");
    dropSuffix("_dif");
    dropSuffix("_albedo");
    dropSuffix("_basecolor");
    dropSuffix("_color");
    dropSuffix("_d");

    if (stem.empty()) stem = "caulk";
    return stem;
}

// ============================================================
// Parse .MTL: newmtl -> map_Kd -> shader
// ============================================================

static unordered_map<string, string> loadMtlToShader(const string& mtlPath)
{
    unordered_map<string, string> out;

    ifstream f(mtlPath);
    if (!f.is_open())
        return out;

    string line;
    string current;

    while (getline(f, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        stringstream ss(line);
        string key;
        ss >> key;

        if (key == "newmtl")
        {
            ss >> current;
        }
        else if (key == "map_Kd")
        {
            string rest;
            getline(ss, rest);
            rest = trim(rest);

            if (!current.empty() && !rest.empty())
                out[current] = shaderFromMapKd(rest);
        }
    }
    return out;
}

// ============================================================
// Parse "v/vt/vn" token
// ============================================================

static bool parseFaceToken(const string& tok, long long& vi, long long& vti, long long& vni)
{
    vi = vti = vni = 0;
    string a, b, c;
    stringstream ss(tok);

    getline(ss, a, '/');
    getline(ss, b, '/');
    getline(ss, c, '/');

    if (a.empty()) return false;

    // OBJ can have negative indices; this exporter ignores them for safety.
    vi = stoll(a);
    if (!b.empty()) vti = stoll(b);
    if (!c.empty()) vni = stoll(c);

    return true;
}

// ============================================================
// OBJ Loader
// If forceCaulkAll == true, we don't need MTL mapping.
// We'll still parse usemtl but ignore it.
// ============================================================

static bool loadOBJ(const string& objPath,
    const unordered_map<string, string>& mtlToShader,
    tGeometry& geometry)
{
    ifstream file(objPath);
    if (!file.is_open())
        return false;

    vector<TVector3> positions;
    vector<TVector2> uvs;
    vector<TVector3> normals;

    string currentUseMtl;
    string currentShader = "caulk";

    string line;
    while (getline(file, line))
    {
        if (line.empty()) continue;

        stringstream ss(line);
        string prefix;
        ss >> prefix;

        if (prefix == "usemtl")
        {
            ss >> currentUseMtl;

            if (forceCaulkAll)
            {
                currentShader = "caulk";
            }
            else
            {
                auto it = mtlToShader.find(currentUseMtl);
                currentShader = (it != mtlToShader.end()) ? it->second : "caulk";
            }
        }
        else if (prefix == "v")
        {
            TVector3 v;
            ss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        }
        else if (prefix == "vt")
        {
            TVector2 uv;
            ss >> uv.x >> uv.y;
            uvs.push_back(uv);
        }
        else if (prefix == "vn")
        {
            TVector3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (prefix == "f")
        {
            vector<size_t> faceVertIndices;
            string tok;

            while (ss >> tok)
            {
                long long vi, vti, vni;
                if (!parseFaceToken(tok, vi, vti, vni))
                    continue;

                // Reject negative indices for safety (you can add support later if you want)
                if (vi <= 0 || (size_t)vi > positions.size())
                    continue;

                TSimple_Vertex vtx;
                vtx.position = positions[(size_t)vi - 1];

                if (vti > 0 && (size_t)vti <= uvs.size())
                    vtx.uv = uvs[(size_t)vti - 1];

                if (vni > 0 && (size_t)vni <= normals.size())
                    vtx.normal = normals[(size_t)vni - 1];

                geometry.vertices.push_back(vtx);
                faceVertIndices.push_back(geometry.vertices.size() - 1);
            }

            // Fan triangulation
            if (faceVertIndices.size() >= 3)
            {
                for (size_t i = 1; i + 1 < faceVertIndices.size(); ++i)
                {
                    Triangle t;
                    t.i0 = faceVertIndices[0];
                    t.i1 = faceVertIndices[i];
                    t.i2 = faceVertIndices[i + 1];
                    t.shader = forceCaulkAll ? "caulk" : currentShader;
                    geometry.tris.push_back(t);
                }
            }
        }
    }

    return true;
}

// ============================================================
// Rotation helpers
// ============================================================

static inline void applyRotateX(float& x, float& y, float& z)
{
    // +90: (x,y,z)->(x,-z, y)
    // -90: (x,y,z)->(x, z,-y)
    // 180:(x,y,z)->(x,-y,-z)
    if (rotateXMode == 1)
    {
        float ny = -z;
        float nz = y;
        y = ny; z = nz;
    }
    else if (rotateXMode == 2)
    {
        float ny = z;
        float nz = -y;
        y = ny; z = nz;
    }
    else if (rotateXMode == 3)
    {
        y = -y;
        z = -z;
    }
}

// ============================================================
// Coordinate conversion
// Y-up OBJ -> Z-up MAP: (x, y, z) -> (x, z, -y) then rotateXMode then scale
// ============================================================

static inline float getScaleFactor()
{
    if (scaleMode == 1) return 1.0f / 2.54f; // divide by 2.54
    if (scaleMode == 2) return 1.0f;         // keep
    return customScale;                      // custom
}

static inline void toMapCoords(const TVector3& in, float& outX, float& outY, float& outZ)
{
    outX = in.x;
    outY = in.z;
    outZ = -in.y;

    applyRotateX(outX, outY, outZ);

    const float s = getScaleFactor();
    outX *= s;
    outY *= s;
    outZ *= s;
}

// Transform normals with same mapping as positions (no scaling)
static inline void transformNormal(const TVector3& nIn, float& nx, float& ny, float& nz)
{
    nx = nIn.x;
    ny = nIn.z;
    nz = -nIn.y;
    applyRotateX(nx, ny, nz);
}

// vector helpers
static inline void vecSub(const float a[3], const float b[3], float out[3])
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static inline void crossProduct(const float a[3], const float b[3], float out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static inline float dot3(const float a[3], const float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline float len3(const float a[3])
{
    return sqrtf(dot3(a, a));
}

static inline void normalize3(float a[3])
{
    float L = len3(a);
    if (L > 1e-8f) { a[0] /= L; a[1] /= L; a[2] /= L; }
}

// ============================================================
// Mesh writer: triangle soup (2x2 meshes) for an arbitrary list of tri IDs
// Auto-winding correction using transformed normals.
// ============================================================

static void writeTriangleListAsMeshes(
    const vector<size_t>& triIds,
    const tGeometry& geometry,
    ofstream& out,
    size_t& brushIndex)
{
    for (size_t tid : triIds)
    {
        const Triangle& tri = geometry.tris[tid];

        const auto& v0 = geometry.vertices[tri.i0];
        const auto& v1 = geometry.vertices[tri.i1];
        const auto& v2 = geometry.vertices[tri.i2];

        float x1, y1, z1, x2, y2, z2, x3, y3, z3;
        toMapCoords(v0.position, x1, y1, z1);
        toMapCoords(v1.position, x2, y2, z2);
        toMapCoords(v2.position, x3, y3, z3);

        bool haveNormal = !(fabs(v0.normal.x) < 1e-8f && fabs(v0.normal.y) < 1e-8f && fabs(v0.normal.z) < 1e-8f
            && fabs(v1.normal.x) < 1e-8f && fabs(v1.normal.y) < 1e-8f && fabs(v1.normal.z) < 1e-8f
            && fabs(v2.normal.x) < 1e-8f && fabs(v2.normal.y) < 1e-8f && fabs(v2.normal.z) < 1e-8f);

        float avgNx = 0, avgNy = 0, avgNz = 0;
        if (haveNormal)
        {
            float tx, ty, tz;
            transformNormal(v0.normal, tx, ty, tz);
            avgNx += tx; avgNy += ty; avgNz += tz;
            transformNormal(v1.normal, tx, ty, tz);
            avgNx += tx; avgNy += ty; avgNz += tz;
            transformNormal(v2.normal, tx, ty, tz);
            avgNx += tx; avgNy += ty; avgNz += tz;

            avgNx /= 3.0f; avgNy /= 3.0f; avgNz /= 3.0f;
            float tmp[3] = { avgNx, avgNy, avgNz };
            normalize3(tmp);
            avgNx = tmp[0]; avgNy = tmp[1]; avgNz = tmp[2];
        }

        float p1[3] = { x1,y1,z1 };
        float p2[3] = { x2,y2,z2 };
        float p3[3] = { x3,y3,z3 };

        float e1[3], e2[3], gN[3];
        vecSub(p2, p1, e1);
        vecSub(p3, p1, e2);
        crossProduct(e1, e2, gN);
        float gLen = len3(gN);
        if (gLen > 1e-9f) { gN[0] /= gLen; gN[1] /= gLen; gN[2] /= gLen; }

        if (haveNormal && gLen > 1e-9f)
        {
            float tN[3] = { avgNx, avgNy, avgNz };
            float dp = dot3(tN, gN);
            if (dp < 0.0f)
            {
                std::swap(x2, x3);
                std::swap(y2, y3);
                std::swap(z2, z3);
            }
        }

        out << "// brush " << brushIndex++ << "\n";
        out << "{\nmesh\n{\n";

        const string outShader = forceCaulkAll ? "caulk" : tri.shader;
        out << outShader << "\n";

        out << "lightmap_gray\n";
        out << "2 2 16 8\n";

        out << "(\n";
        out << "v " << x1 << " " << y1 << " " << z1 << " t 0 0 0 0\n";
        out << "v " << x1 << " " << y1 << " " << z1 << " t 0 0 0 0\n";
        out << ")\n";

        out << "(\n";
        out << "v " << x2 << " " << y2 << " " << z2 << " t 0 0 0 0\n";
        out << "v " << x3 << " " << y3 << " " << z3 << " t 0 0 0 0\n";
        out << ")\n";

        out << "}\n}\n";
    }
}

// ============================================================
// Write MAP file (OBJ export) - MESH ONLY
// ============================================================

static void writeMapFile_OBJ(size_t startTri,
    size_t endTri,
    const tGeometry& geometry,
    const string& baseName,
    int partIndex)
{
    stringstream filename;
    filename << "data\\" << baseName;
    if (partIndex > 0)
        filename << "_" << setw(2) << setfill('0') << partIndex;
    filename << ".Map";

    const string outPath = filename.str();
    const string tmpPath = outPath + ".tmp";

    unordered_map<string, vector<size_t>> trisByShader;
    trisByShader.reserve(256);

    for (size_t t = startTri; t < endTri; ++t)
    {
        const Triangle& tri = geometry.tris[t];
        const string key = forceCaulkAll ? "caulk" : tri.shader;
        trisByShader[key].push_back(t);
    }

    ofstream out(tmpPath);
    if (!out.is_open())
        return;

    out << "iwmap 4\n";
    out << "// entity 0\n";
    out << "{\n\n";
    out << "\"classname\" \"worldspawn\"\n";

    size_t brushIndex = 0;

    for (auto& kv : trisByShader)
    {
        vector<size_t>& triIds = kv.second;
        writeTriangleListAsMeshes(triIds, geometry, out, brushIndex);
    }

    out << "}\n";
    out.close();

    replaceFileAtomicA(outPath, tmpPath);
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    cout << "OBJ TO COD MAP 1.2\n";
    cout << "RotateX + Auto-winding for mesh output\n\n";

    // SCALE MODE (3 options). If custom, do not ask 1/2 follow-ups.
    scaleMode = askIntInRange(
        "Scale mode:\n"
        "  1 = Divide by 2.54 (HUSKY export -> COD scale)\n"
        "  2 = Keep original scale\n"
        "  3 = Custom scale factor\n> ",
        1, 3);

    if (scaleMode == 3)
    {
        // Reasonable guard range; adjust if you want.
        customScale = askFloatInRange(
            "\nEnter custom scale factor (Downscale down to 0.0001 or upscale up to 100).\n> ",
            0.0001f, 100.0f);
        cout << "Custom scale set to: " << customScale << "\n";
    }

    // SPLIT MODE: user picks 1 or 2, if 2 then ask 2..10
    int splitMode = askIntInRange(
        "\nOutput files:\n"
        "  1 = Single .map file\n"
        "  2 = Split into multiple files\n> ",
        1, 2);

    int splitCount = 1;
    if (splitMode == 2)
    {
        splitCount = askIntInRange(
            "\nHow many files do you want to split into? (pick between 2 up to 10 seperate files)\n> ",
            2, 10);
    }

    rotateXMode = askIntInRange(
        "\nRotate output about X axis:\n"
        "  0 = none\n"
        "  1 = +90 degrees (fixes Husky sideways output)\n"
        "  2 = -90 degrees\n"
        "  3 = 180 degrees\n> ",
        0, 3);

    int caulkChoice = askIntInRange(
        "\nSurface mode:\n"
        "  1 = Keep original shaders (requires MTL)\n"
        "  2 = Force CAULK on all surfaces (skip MTL)\n> ",
        1, 2);

    forceCaulkAll = (caulkChoice == 2);

    unordered_map<string, string> mtlToShader;

    // If not autocaulk, prompt for MTL and validate file
    if (!forceCaulkAll)
    {
        string mtlPath = promptForDroppedFile(
            "\nDrag the MTL file and press Enter:\n> ",
            { ".mtl" });

        cout << "\nLoading MTL...\n";
        mtlToShader = loadMtlToShader(mtlPath);
        if (mtlToShader.empty())
            cout << "Warning: No materials parsed from MTL. Using caulk for unknown materials.\n";
    }
    else
    {
        cout << "\nAutoCaulk enabled: skipping MTL prompt.\n";
    }

    // Prompt for OBJ (validate extension + existence)
    string objPath = promptForDroppedFile(
        "\nDrag the OBJ file and press Enter:\n> ",
        { ".obj" });

    cout << "\nLoading OBJ...\n";
    tGeometry geometry;
    if (!loadOBJ(objPath, mtlToShader, geometry))
    {
        cout << "Failed to open OBJ file.\n";
        system("pause");
        return 1;
    }

    if (geometry.tris.empty() || geometry.vertices.empty())
    {
        cout << "OBJ loaded, but no usable geometry was found (0 tris or 0 verts).\n";
        system("pause");
        return 1;
    }

    cout << "Loaded " << geometry.vertices.size() << " vertices and "
        << geometry.tris.size() << " triangles.\n";

    CreateDirectoryA("data", NULL);

    string baseName = getStemNoFilesystem(objPath);
    size_t totalTris = geometry.tris.size();

    if (splitCount == 1)
    {
        writeMapFile_OBJ(0, totalTris, geometry, baseName, 0);
    }
    else
    {
        size_t chunk = (totalTris + (size_t)splitCount - 1) / (size_t)splitCount;
        for (int part = 0; part < splitCount; ++part)
        {
            size_t start = (size_t)part * chunk;
            size_t end = min(totalTris, start + chunk);
            if (start >= end) break;
            writeMapFile_OBJ(start, end, geometry, baseName, part + 1);
        }
    }

    cout << "\nExport complete.\n";
    cout << "Files are in the DATA folder.\n";
    system("pause");
    return 0;
}
