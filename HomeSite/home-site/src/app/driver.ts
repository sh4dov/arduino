export interface Light {
    name: string;
    id: number;
    auto: boolean;
    on: boolean;
    brightness: number;
}

export interface Driver {
    name: string;
    ip: string;
    lights: Light[];
    from: number;
    to: number;
    online: boolean;
}

export const drivers: Driver[] = [
    {
        name: "Kuchnia dół",
        ip: "192.168.100.32",
        from: 7,
        to: 16,
        online: true,
        lights: [
            {
                name: "Oswietlenie górne",
                id: 1,
                auto: true,
                on: true,
                brightness: 100
            },
            {
                name: "Oswietlenie szafek",
                id: 2,
                auto: false,
                on: false,
                brightness: 100
            },
            {
                name: "Oswietlenie blat",
                id: 3,
                auto: false,
                on: false,
                brightness: 100
            },
            {
                name: "Oswietlenie podłoga",
                id: 4,
                auto: true,
                on: false,
                brightness: 100
            }
        ]
    },
    {
        name: "Korytarz dół",
        ip: "192.168.100.33",
        from: 7,
        to: 16,
        online: false,
        lights: [
            {
                name: "Plafon",
                id: 1,
                auto: true,
                on: false,
                brightness: 100
            },
            {
                name: "Główne",
                id: 2,
                auto: true,
                on: false,
                brightness: 100
            }
        ]
    },
    {
        name: "Korytarz góra",
        ip: "192.168.100.34",
        from: 7,
        to: 16,
        online: false,
        lights: [
            {
                name: "Plafon",
                id: 1,
                auto: true,
                on: false,
                brightness: 50
            },
            {
                name: "Główne",
                id: 2,
                auto: true,
                on: false,
                brightness: 50
            }
        ]
    }
];