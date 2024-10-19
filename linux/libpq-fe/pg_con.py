import psycopg2
from psycopg2 import sql

def connect(host, port, user, password, database):
    conn = psycopg2.connect(
                host=host,
                database=database,
                user=user,
                password=password,
                port=port
    )
    print("connected!")
    return conn

def write_blob(conn, path_to_file, name):
    """ insert a BLOB into a table """
    print(path_to_file + name)
    try:
        # read data from a picture
        with open(path_to_file + name, 'rb') as file:
            data = file.read()

        # create a new cursor object`
        cur = conn.cursor()

        # execute the INSERT statement
        cur.execute("INSERT INTO logs(name, modeling) " +
                    "VALUES(%s, %s)",
                    (name, psycopg2.Binary(data),))
        # commit the changes to the database
        conn.commit()
        # close the communication with the PostgresQL database
        cur.close()
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()

def read_blob(conn, name):
    blob = None
    try:
        # create a new cursor object
        cur = conn.cursor()
        # execute the SELECT statement
        cur.execute("SELECT * " +
                    "FROM logs l " +
                    "WHERE l.name = %s ",
                    (name,))

        blob = cur.fetchone()
        cur.close()
    
    except (Exception, psycopg2.DatabaseError) as error:
        print(error)
    finally:
        if conn is not None:
            conn.close()
    return blob

if __name__ == "__main__":
    conn = connect("localhost", 5432, "postgres", "11223344", "postgres")
    #write_blob(conn, "./", "3d_modeling.ply")
    blob = read_blob(conn, "3d_modeling.ply")
    print(blob)
    with open('read/3d_modeling.ply', 'wb') as file:
            file.write(blob[2]) 