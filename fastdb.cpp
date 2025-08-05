#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sqlite3.h>
#include <iomanip>

using namespace std;

// Tipos y modificadores soportados
const vector<string> FIELD_TYPES = {"--int", "--string", "--float", "--bool", "--date", "--blob", "--text"};
const vector<string> FIELD_MODIFIERS = {"--pk", "--ai", "--notnull", "--unique", "--default", "--fk", "--ondelete", "--onupdate"};
const vector<string> FK_ACTIONS = {"cascade", "restrict", "setnull", "setdefault", "noaction"};

sqlite3* db = nullptr;

void show_help() {
    cout << "FastDB CLI - Herramienta completa para SQLite\n\n"
         << "Uso general:\n"
         << "  fastdb --db <archivo.db> [comando] [opciones]\n\n"
         << "Comandos disponibles:\n"
         << "  create --table <nombre> fields <definiciones>\n"
         << "  insert --table <nombre> values <valores>\n"
         << "  update --table <nombre> set <campo=valor> where <condicion>\n"
         << "  delete --table <nombre> where <condicion>\n"
         << "  select [*|campos] from <tabla> [join] [where] [group] [order] [limit]\n\n"
         << "Ejemplos completos:\n"
         << "  Creación de tabla con FK y acciones:\n"
         << "    fastdb --db app.db create --table clientes fields \\\n"
         << "      --int id --pk --ai \\\n"
         << "      --string nombre --notnull \\\n"
         << "      --int pais_id --fk paises(id) --ondelete cascade\n\n"
         << "  Consultas con joins:\n"
         << "    fastdb --db app.db select c.*, p.nombre from clientes c \\\n"
         << "      join paises p on c.pais_id = p.id \\\n"
         << "      where p.continente='America'\n\n"
         << "  Transacciones:\n"
         << "    fastdb --db app.db begin\n"
         << "    fastdb --db app.db insert --table ventas values (1, 100.50)\n"
         << "    fastdb --db app.db commit\n";
}

bool execute_sql(const string& sql, bool show_results = true) {
    char* errMsg = nullptr;
    
    if (show_results) {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            cerr << "Error en SQL: " << sqlite3_errmsg(db) << endl;
            return false;
        }
        
        int cols = sqlite3_column_count(stmt);
        
        // Mostrar encabezados
        for (int i = 0; i < cols; i++) {
            cout << left << setw(20) << sqlite3_column_name(stmt, i) << " | ";
        }
        cout << "\n" << string(cols * 22, '-') << "\n";
        
        // Mostrar datos
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            for (int i = 0; i < cols; i++) {
                const unsigned char* val = sqlite3_column_text(stmt, i);
                cout << left << setw(20) << (val ? reinterpret_cast<const char*>(val) : "NULL") << " | ";
            }
            cout << "\n";
        }
        
        sqlite3_finalize(stmt);
        return true;
    }
    else {
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            cerr << "Error en SQL: " << errMsg << endl;
            sqlite3_free(errMsg);
            return false;
        }
        return true;
    }
}

string build_create_table_sql(const string& table_name, const vector<string>& fields) {
    string sql = "CREATE TABLE " + table_name + " (\n";
    vector<string> columns;
    vector<string> constraints;
    
    for (size_t i = 0; i < fields.size(); ) {
        // 1. Tipo de campo
        if (find(FIELD_TYPES.begin(), FIELD_TYPES.end(), fields[i]) == FIELD_TYPES.end()) {
            throw runtime_error("Tipo de campo inválido: " + fields[i]);
        }
        string field_type = fields[i].substr(2);
        i++;
        
        // 2. Nombre del campo
        if (i >= fields.size()) {
            throw runtime_error("Falta nombre de campo después de: " + fields[i-1]);
        }
        string field_name = fields[i];
        i++;
        
        // 3. Modificadores
        vector<string> modifiers;
        string fk_table, fk_column, on_delete, on_update;
        
        while (i < fields.size() && find(FIELD_MODIFIERS.begin(), FIELD_MODIFIERS.end(), fields[i]) != FIELD_MODIFIERS.end()) {
            string mod = fields[i];
            
            if (mod == "--fk") {
                if (i + 1 >= fields.size()) {
                    throw runtime_error("Falta especificación de FK después de --fk");
                }
                string fk_spec = fields[++i];
                if (fk_spec.front() == '(' && fk_spec.back() == ')') {
                    fk_spec = fk_spec.substr(1, fk_spec.size() - 2);
                }
                size_t dot = fk_spec.find('.');
                size_t paren = fk_spec.find('(');
                
                if (dot != string::npos) {
                    fk_table = fk_spec.substr(0, dot);
                    fk_column = fk_spec.substr(dot + 1);
                }
                else if (paren != string::npos) {
                    fk_table = fk_spec.substr(0, paren);
                    fk_column = fk_spec.substr(paren + 1, fk_spec.size() - paren - 2);
                }
                else {
                    size_t sep = fk_spec.find(',');
                    if (sep == string::npos) sep = fk_spec.find(' ');
                    if (sep == string::npos) {
                        throw runtime_error("Formato de FK inválido. Use tabla(columna) o tabla.columna");
                    }
                    fk_table = fk_spec.substr(0, sep);
                    fk_column = fk_spec.substr(sep + 1);
                }
            }
            else if (mod == "--ondelete") {
                if (i + 1 >= fields.size()) {
                    throw runtime_error("Falta acción después de --ondelete");
                }
                on_delete = fields[++i];
                if (find(FK_ACTIONS.begin(), FK_ACTIONS.end(), on_delete) == FK_ACTIONS.end()) {
                    throw runtime_error("Acción FK inválida: " + on_delete);
                }
            }
            else if (mod == "--onupdate") {
                if (i + 1 >= fields.size()) {
                    throw runtime_error("Falta acción después de --onupdate");
                }
                on_update = fields[++i];
                if (find(FK_ACTIONS.begin(), FK_ACTIONS.end(), on_update) == FK_ACTIONS.end()) {
                    throw runtime_error("Acción FK inválida: " + on_update);
                }
            }
            else {
                modifiers.push_back(mod);
            }
            i++;
        }
        
        // Construir definición de columna
        string column_def = "  " + field_name + " ";
        
        // Mapear tipos a SQLite
        if (field_type == "int") column_def += "INTEGER";
        else if (field_type == "float") column_def += "REAL";
        else if (field_type == "bool") column_def += "INTEGER";
        else column_def += "TEXT";
        
        // Aplicar modificadores
        bool is_pk = false;
        for (const auto& mod : modifiers) {
            if (mod == "--pk") {
                column_def += " PRIMARY KEY";
                is_pk = true;
            }
            else if (mod == "--ai") column_def += " AUTOINCREMENT";
            else if (mod == "--notnull") column_def += " NOT NULL";
            else if (mod == "--unique") column_def += " UNIQUE";
            else if (mod == "--default" && i < fields.size()) {
                column_def += " DEFAULT " + fields[i++];
            }
        }
        
        columns.push_back(column_def);
        
        // Construir constraints de FK
        if (!fk_table.empty()) {
            string fk_constraint = "FOREIGN KEY(" + field_name + ") REFERENCES " + fk_table + "(" + fk_column + ")";
            if (!on_delete.empty()) fk_constraint += " ON DELETE " + on_delete;
            if (!on_update.empty()) fk_constraint += " ON UPDATE " + on_update;
            constraints.push_back(fk_constraint);
        }
    }
    
    // Combinar todo
    for (size_t i = 0; i < columns.size(); i++) {
        sql += columns[i];
        if (i < columns.size() - 1 || !constraints.empty()) {
            sql += ",\n";
        }
    }
    
    for (size_t i = 0; i < constraints.size(); i++) {
        sql += "  " + constraints[i];
        if (i < constraints.size() - 1) {
            sql += ",\n";
        }
    }
    
    sql += "\n);";
    return sql;
}

void handle_create(const vector<string>& args) {
    if (args.size() < 4 || args[0] != "--table" || args[2] != "fields") {
        throw runtime_error("Sintaxis inválida. Uso: create --table <nombre> fields <definiciones>");
    }
    
    string table_name = args[1];
    vector<string> field_defs(args.begin() + 3, args.end());
    
    string sql = build_create_table_sql(table_name, field_defs);
    cout << "SQL:\n" << sql << "\n";
    
    if (!execute_sql(sql, false)) {
        throw runtime_error("Error al crear tabla");
    }
    
    cout << "Tabla '" << table_name << "' creada exitosamente!\n";
}

void handle_insert(const vector<string>& args) {
    if (args.size() < 4 || args[0] != "--table" || args[2] != "values") {
        throw runtime_error("Sintaxis inválida. Uso: insert --table <nombre> values <valores>");
    }
    
    string table_name = args[1];
    string values = args[3];
    
    // Simplificación - en realidad deberías parsear los valores correctamente
    string sql = "INSERT INTO " + table_name + " VALUES (" + values + ");";
    
    if (!execute_sql(sql, false)) {
        throw runtime_error("Error al insertar datos");
    }
    
    cout << "Datos insertados en '" << table_name << "'\n";
}

void handle_update(const vector<string>& args) {
    if (args.size() < 6 || args[0] != "--table" || args[2] != "set" || args[4] != "where") {
        throw runtime_error("Sintaxis inválida. Uso: update --table <nombre> set <campo=valor> where <condicion>");
    }
    
    string table_name = args[1];
    string set_clause = args[3];
    string where_clause = args[5];
    
    string sql = "UPDATE " + table_name + " SET " + set_clause + " WHERE " + where_clause + ";";
    
    if (!execute_sql(sql, false)) {
        throw runtime_error("Error al actualizar datos");
    }
    
    cout << "Datos actualizados en '" << table_name << "'\n";
}

void handle_delete(const vector<string>& args) {
    if (args.size() < 4 || args[0] != "--table" || args[2] != "where") {
        throw runtime_error("Sintaxis inválida. Uso: delete --table <nombre> where <condicion>");
    }
    
    string table_name = args[1];
    string where_clause = args[3];
    
    string sql = "DELETE FROM " + table_name + " WHERE " + where_clause + ";";
    
    if (!execute_sql(sql, false)) {
        throw runtime_error("Error al eliminar datos");
    }
    
    cout << "Datos eliminados de '" << table_name << "'\n";
}

void handle_select(const vector<string>& args) {
    if (args.size() < 3 || args[1] != "from") {
        throw runtime_error("Sintaxis inválida. Uso: select [*|campos] from <tabla> [where <condicion>]");
    }
    
    string fields = args[0];
    string table_name = args[2];
    string where_clause = "";
    string join_clause = "";
    
    // Parsear condiciones adicionales (simplificado)
    for (size_t i = 3; i < args.size(); i++) {
        if (i + 1 < args.size() && args[i] == "where") {
            where_clause = " WHERE " + args[i+1];
            i++;
        }
        else if (i + 3 < args.size() && args[i] == "join") {
            join_clause += " JOIN " + args[i+1] + " ON " + args[i+3];
            i += 3;
        }
    }
    
    string sql = "SELECT " + fields + " FROM " + table_name + join_clause + where_clause + ";";
    cout << "SQL:\n" << sql << "\n";
    
    if (!execute_sql(sql, true)) {
        throw runtime_error("Error en la consulta");
    }
}

void handle_transaction(const string& action) {
    string sql = action + ";";
    if (!execute_sql(sql, false)) {
        throw runtime_error("Error en transacción");
    }
    cout << "Transacción '" << action << "' ejecutada\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    vector<string> args(argv + 1, argv + argc);
    
    try {
        // Verificar --db
        if (args[0] != "--db" || args.size() < 2) {
            throw runtime_error("Debe especificar base de datos con --db <archivo.db>");
        }
        
        string db_file = args[1];
        if (sqlite3_open(db_file.c_str(), &db) != SQLITE_OK) {
            throw runtime_error("No se pudo abrir la base de datos: " + string(sqlite3_errmsg(db)));
        }
        
        args.erase(args.begin(), args.begin() + 2);
        
        if (args.empty()) {
            show_help();
            return 0;
        }
        
        string command = args[0];
        args.erase(args.begin());
        
        if (command == "create") {
            handle_create(args);
        }
        else if (command == "insert") {
            handle_insert(args);
        }
        else if (command == "update") {
            handle_update(args);
        }
        else if (command == "delete") {
            handle_delete(args);
        }
        else if (command == "select") {
            handle_select(args);
        }
        else if (command == "begin" || command == "commit" || command == "rollback") {
            handle_transaction(command);
        }
        else {
            throw runtime_error("Comando no reconocido: " + command);
        }
        
        sqlite3_close(db);
    } 
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        show_help();
        if (db) sqlite3_close(db);
        return 1;
    }
    
    return 0;
}
