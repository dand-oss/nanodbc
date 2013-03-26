#include "nanodbc.h"
#include <iostream>
#include <vector>

using namespace std;

void show(nanodbc::result& results);
nanodbc::result execute(nanodbc::connection& connection, const string& query);

int main(int argc, char* argv[])
{
    // Establishing connections
    const string connection_string = argv[1];
    nanodbc::connection connection(connection_string);
    // or connection(connection_string, timeout_seconds);
    // or connection("data source name", "username", "password");
    // or connection("data source name", "username", "password", timeout_seconds);
    cout << "Connected with driver " << connection.driver_name() << endl;

    // Direct execution
    execute(connection, "drop table if exists public.simple_test;");
    execute(connection, "create table public.simple_test (a int, b varchar(10));");
    execute(connection, "insert into public.simple_test values (1, 'one');");
    execute(connection, "insert into public.simple_test values (2, 'two');");
    execute(connection, "insert into public.simple_test values (3, 'tri');");
    execute(connection, "insert into public.simple_test (b) values ('z');");
    nanodbc::result results = execute(connection, "select * from public.simple_test;");
    show(results);

    // Binding parameters
    {
        nanodbc::statement statement;
        statement.prepare(connection, "insert into public.simple_test (a, b) values (?, ?);");
        const int eight_int = 8;
        statement.bind_parameter(0, &eight_int);
        const string eight_str = "eight";
        statement.bind_parameter(1, eight_str.c_str());
        statement.execute();

        statement.prepare(connection, "select * from public.simple_test where a = ?;");
        statement.bind_parameter(0, &eight_int);
        results = statement.execute();
        show(results);
    }

    // Transactions
    {
        cout << "\ndeleting all rows ... " << flush;
        nanodbc::transaction transaction(connection);
        execute(connection, "delete from public.simple_test;");
        // transaction will be rolled back if we don't call transaction.commit()
    }
    results = execute(connection, "select count(1) from public.simple_test;");
    results.next();
    cout << "still have " << results.get<int>(0) << " rows!" << endl;

    // Batch inserting
    {
        nanodbc::statement statement;
        execute(connection, "drop table if exists public.batch_test;");
        execute(connection, "create table public.batch_test (x varchar(10), y int, z float);");
        statement.prepare(connection, "insert into public.batch_test (x, y, z) values (?, ?, ?);");

        const char xdata[][10] = {"this", "is", "a", "test"};
        statement.bind_parameter(0, xdata);
        
        int ydata[] = { 1, 2, 3, 4 };
        statement.bind_parameter(1, ydata);

        float zdata[] = { 1.1, 2.2, 3.3, 4.4 };
        statement.bind_parameter(2, zdata);

        nanodbc::transaction transaction(connection);
        statement.execute(4);
        transaction.commit();

        results = statement.execute_direct(connection, "select * from public.batch_test;", 3);
        show(results);
    }

    // Dates and Times
    {
        execute(connection, "drop table if exists public.date_test;");
        execute(connection, "create table public.date_test (x datetime);");
        execute(connection, "insert into public.date_test values (current_timestamp);");
        results = execute(connection, "select * from public.date_test;");
        results.next();
        nanodbc::date date = results.get<nanodbc::date>(0);
        cout << endl << date.year << "-" << date.month << "-" << date.day << endl;
        results = execute(connection, "select * from public.date_test;");
        show(results);
    }

    // Cleanup
    execute(connection, "drop table if exists public.simple_test;");
    execute(connection, "drop table if exists public.batch_test;");
    execute(connection, "drop table if exists public.date_test;");
}

void show(nanodbc::result& results)
{
    const short columns = results.columns();
    long rows_displayed = 0;

    cout << "\nDisplaying " << results.affected_rows() << " rows "
         << "(" << results.rowset_size() << " fetched at a time):" << endl;

    // show the column names
    cout << "row\t";
    for(short i = 0; i < columns; ++i)
        cout << results.column_name(i) << "\t";
    cout << endl;

    // show the column data for each row
    while(results.next())
    {
        cout << rows_displayed++ << "\t";
        for(short col = 0; col < columns; ++col)
        {
            if(results.is_null(col))
                cout << "(null)";
            else
                cout << "(" << results.get<string>(col) << ")";
            cout << "\t";
        }
        cout << endl;
    }
}

nanodbc::result execute(nanodbc::connection& connection, const string& query)
{
    nanodbc::statement statement;
    return statement.execute_direct(connection, query);
}
