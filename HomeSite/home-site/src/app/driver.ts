export interface Light {
    name: string;
    id: number;
    auto: boolean;
    on: boolean;
    brightness: number;
    loading: boolean;
}

export interface Driver {
    name: string;
    ip: string;
    lights: Light[];
    from: number;
    to: number;
    online: boolean;
    error: string;
    id: number;
}
