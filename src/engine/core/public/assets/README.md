# Asset management

Assets are virtually stored into `Package`.
Each package use a valid asset registry. Asset registries handle asset allocations. By default, packages are using the same global asset registry, but for some
use cases it could be helpfull to specify a custom asset registry (which allocate assets contiguously inside)

Packages may be transient. Transient packages will not allow asset serialization or loading from file.
Theses are only valid for temporary assets that will be deleted when the engine shut down.

Packages are virtual interfaces that can be implemented to store and fetch data from the native filesystem, but also for example compressed archives, or web server...