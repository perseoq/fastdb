### 1. Creación de Tablas Avanzadas
```bash
# Tabla con clave primaria y autoincremento
fastdb --db empresa.db create --table empleados fields \
  --int id --pk --ai \
  --string nombre --notnull \
  --string puesto \
  --float salario --default 0.0

# Tabla con clave foránea y acciones en cascada
fastdb --db empresa.db create --table departamentos fields \
  --int id --pk --ai \
  --string nombre --unique \
  --int gerente_id --fk empleados(id) --ondelete setnull

# Tabla con múltiples constraints
fastdb --db empresa.db create --table proyectos fields \
  --int id --pk \
  --string nombre --notnull --unique \
  --date fecha_inicio \
  --date fecha_fin \
  --int departamento_id --fk departamentos(id) --ondelete cascade
```

### 2. Operaciones INSERT
```bash
# Insertar un registro
fastdb --db empresa.db insert --table empleados values \
  "1,'Juan Pérez','Gerente',5000.00"

# Insertar múltiples valores (usando transacción)
fastdb --db empresa.db begin
fastdb --db empresa.db insert --table empleados values "2,'María López','Desarrollador',3000.00"
fastdb --db empresa.db insert --table empleados values "3,'Carlos Ruiz','Diseñador',2800.00"
fastdb --db empresa.db commit
```

### 3. Consultas SELECT con Joins
```bash
# Consulta simple
fastdb --db empresa.db select * from empleados

# Consulta con condición
fastdb --db empresa.db select nombre, puesto from empleados where salario > 4000

# INNER JOIN
fastdb --db empresa.db select e.nombre, d.nombre from empleados e \
  join departamentos d on e.departamento_id = d.id

# LEFT JOIN con múltiples condiciones
fastdb --db empresa.db select p.nombre, e.nombre from proyectos p \
  left join empleados e on p.responsable_id = e.id \
  where p.fecha_fin > '2023-12-31'
```

### 4. Operaciones UPDATE
```bash
# Actualización simple
fastdb --db empresa.db update --table empleados set salario=salario*1.1 where puesto='Desarrollador'

# Actualización con subconsulta
fastdb --db empresa.db update --table empleados set departamento_id=3 \
  where id in (select empleado_id from asignaciones where proyecto_id=5)
```

### 5. Operaciones DELETE
```bash
# Eliminar registros específicos
fastdb --db empresa.db delete --table empleados where id=10

# Eliminar con join implícito
fastdb --db empresa.db delete --table proyectos where departamento_id in \
  (select id from departamentos where nombre='Ventas')
```

### 6. Transacciones Complejas
```bash
# Transacción con rollback condicional
fastdb --db empresa.db begin
fastdb --db empresa.db update --table cuentas set saldo=saldo-100 where id=1
fastdb --db empresa.db update --table cuentas set saldo=saldo+100 where id=2
fastdb --db empresa.db select saldo from cuentas where id=1
# Si el saldo es negativo:
fastdb --db empresa.db rollback
# Si todo está bien:
fastdb --db empresa.db commit
```

### 7. Consultas Avanzadas
```bash
# GROUP BY y funciones de agregación
fastdb --db empresa.db select departamento_id, avg(salario), count(*) from empleados \
  group by departamento_id

# ORDER BY y LIMIT
fastdb --db empresa.db select * from empleados order by salario desc limit 5

# Subconsultas
fastdb --db empresa.db select nombre from empleados where salario > \
  (select avg(salario) from empleados)
```

### 8. Manejo de Constraints
```bash
# Tabla con constraint CHECK
fastdb --db empresa.db create --table productos fields \
  --int id --pk \
  --string nombre \
  --float precio --check "precio>0" \
  --int stock --check "stock>=0"

# Tabla con UNIQUE compuesto
fastdb --db empresa.db create --table asignaciones fields \
  --int empleado_id --fk empleados(id) \
  --int proyecto_id --fk proyectos(id) \
  --date fecha_asignacion \
  --unique "empleado_id,proyecto_id"
```

### 9. Backup y Restauración
```bash
# Desde la línea de comandos (usando SQLite directamente)
sqlite3 empresa.db ".backup empresa_backup.db"
sqlite3 empresa_restaurada.db ".restore empresa_backup.db"
```

### 10. Ejemplo Completo de Aplicación
```bash
# Crear estructura completa de una aplicación de blog
fastdb --db blog.db create --table usuarios fields \
  --int id --pk --ai \
  --string username --unique --notnull \
  --string email --unique \
  --date fecha_registro --default "datetime('now')"

fastdb --db blog.db create --table categorias fields \
  --int id --pk --ai \
  --string nombre --notnull \
  --string slug --unique

fastdb --db blog.db create --table posts fields \
  --int id --pk --ai \
  --string titulo --notnull \
  --string contenido --text \
  --int autor_id --fk usuarios(id) --ondelete cascade \
  --int categoria_id --fk categorias(id) --ondelete setnull \
  --date fecha_publicacion --default "datetime('now')" \
  --bool publicado --default 0

# Insertar datos iniciales
fastdb --db blog.db insert --table categorias values "1,'Tecnología','tecnologia'"
fastdb --db blog.db insert --table usuarios values "1,'admin','admin@example.com',datetime('now')"

# Consulta compleja: posts con autor y categoría
fastdb --db blog.db select p.titulo, u.username, c.nombre from posts p \
  join usuarios u on p.autor_id = u.id \
  left join categorias c on p.categoria_id = c.id \
  where p.publicado = 1 \
  order by p.fecha_publicacion desc
```

### Tips Adicionales:
1. Para valores con comas, usar comillas:
   ```bash
   fastdb --db ejemplo.db insert --table productos values '1,"Producto, especial",25.99'
   ```

2. Usar `--help` para ver la ayuda en cualquier momento:
   ```bash
   fastdb --help
   ```

3. Para operaciones masivas, considerar usar un archivo SQL:
   ```bash
   fastdb --db empresa.db < operaciones.sql
   ```

4. Las transacciones mejoran el rendimiento en operaciones masivas:
   ```bash
   fastdb --db datos.db begin
   # Muchas operaciones INSERT/UPDATE
   fastdb --db datos.db commit
   ```
