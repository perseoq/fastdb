FastDB CLI - Herramienta completa para SQLite
```sh
Uso general:
  fastdb --db <archivo.db> [comando] [opciones]

Comandos disponibles:
  create --table <nombre> fields <definiciones>
  insert --table <nombre> values <valores>
  update --table <nombre> set <campo=valor> where <condicion>
  delete --table <nombre> where <condicion>
  select [*|campos] from <tabla> [join] [where] [group] [order] [limit]

Ejemplos completos:
  Creación de tabla con FK y acciones:
    fastdb --db app.db create --table clientes fields \
      --int id --pk --ai \
      --string nombre --notnull \
      --int pais_id --fk paises(id) --ondelete cascade

  Consultas con joins:
    fastdb --db app.db select c.*, p.nombre from clientes c \
      join paises p on c.pais_id = p.id \
      where p.continente='America'

  Transacciones:
    fastdb --db app.db begin
    fastdb --db app.db insert --table ventas values (1, 100.50)
    fastdb --db app.db commit
```

## Instrucciones para Compilar

```bash
g++ -std=c++17 -o fastdb fastdb.cpp -lsqlite3
sudo mv fastdb /usr/local/bin/
```

## Características Implementadas

1. **Operaciones CRUD completas**:
   - `CREATE TABLE` con soporte para constraints avanzadas
   - `INSERT` de datos
   - `UPDATE` con cláusula WHERE
   - `DELETE` con condiciones
   - `SELECT` con resultados formateados

2. **Soporte para relaciones**:
   - Claves foráneas (`--fk`)
   - Acciones ON DELETE y ON UPDATE (`cascade`, `set null`, etc.)
   - Soporte para joins implícito en consultas

3. **Manejo de transacciones**:
   - `BEGIN`, `COMMIT`, `ROLLBACK`

4. **Soporte para diferentes tipos de datos**:
   - Conversión automática a tipos SQLite
   - Validación de tipos y modificadores

## Ejemplos de Uso

1. **Creación de tablas con FK**:
```bash
./fastdb --db app.db create --table clientes fields \
  --int id --pk --ai \
  --string nombre --notnull \
  --int pais_id --fk paises(id) --ondelete cascade
```

2. **Consultas con joins**:
```bash
./fastdb --db app.db select c.nombre, p.nombre from clientes c \
  join paises p on c.pais_id = p.id \
  where p.continente='America'
```

3. **Transacciones**:
```bash
./fastdb --db app.db begin
./fastdb --db app.db insert --table ventas values (1, 100.50)
./fastdb --db app.db commit
```

4. **Actualizaciones condicionales**:
```bash
./fastdb --db app.db update --table productos set precio=precio*1.1 where categoria='Electrónica'
```


> PARA SABER MÁS [LEE EL MANUAL DE USO](MANUAL.md)
